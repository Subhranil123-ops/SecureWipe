#include "EvidenceValidator.h"
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <iterator>
#include <jpeglib.h>
#include <setjmp.h>

bool EvidenceValidator::validateHeader(const std::string &filePath) const
{
    std::ifstream file(filePath, std::ios::binary);

    if (!file)
        return false;

    std::uint8_t header[3];

    file.read(reinterpret_cast<char *>(header), sizeof(header));

    if (file.gcount() != sizeof(header))
        return false;

    return header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF;
}

bool EvidenceValidator::validateFooter(const std::string &filePath) const
{
    std::ifstream file(filePath, std::ios::binary);

    if (!file)
        return false;

    // Move to the end of the file
    file.seekg(0, std::ios::end);

    // Get the file size
    std::streamoff fileSize = file.tellg();

    if (fileSize < 2)
        return false;

    file.seekg(-2, std::ios::end);

    std::uint8_t footer[2];

    file.read(reinterpret_cast<char *>(footer), sizeof(footer));

    if (file.gcount() != sizeof(footer))
        return false;

    return footer[0] == 0xFF && footer[1] == 0xD9;
}

bool EvidenceValidator::validateSize(const EvidenceItem &item) const
{
    if (item.recoveredPath.empty())
        return false;
    try
    {
        std::uintmax_t actualSize = std::filesystem::file_size(item.recoveredPath);

        return actualSize == item.size;
    }
    catch (const std::filesystem::filesystem_error &)
    {
        return false;
    }
}

bool EvidenceValidator::validateStructure(
    const std::string &filePath) const
{
    std::ifstream file(filePath, std::ios::binary);

    if (!file)
        return false;

    // Move to the end of the file
    file.seekg(0, std::ios::end);

    // Get file size
    std::streamoff fileSize = file.tellg();

    // JPEG must contain more than just a few bytes
    if (fileSize < 5)
        return false;

    // Move back to the beginning
    file.seekg(0, std::ios::beg);

    // Store the complete file in memory
    std::vector<std::uint8_t> data(
        static_cast<std::size_t>(fileSize));

    // Read the complete file
    file.read(
        reinterpret_cast<char *>(data.data()),
        static_cast<std::streamsize>(data.size()));

    // Make sure the complete file was read
    if (file.gcount() !=
        static_cast<std::streamsize>(data.size()))
    {
        return false;
    }

    // JPEG Start Of Image (SOI)
    if (data[0] != 0xFF ||
        data[1] != 0xD8)
    {
        return false;
    }

    std::size_t offset = 2;

    while (offset < data.size())
    {
        // JPEG markers start with FF
        if (data[offset] != 0xFF)
            return false;

        // Skip repeated FF bytes
        while (offset < data.size() &&
               data[offset] == 0xFF)
        {
            ++offset;
        }

        if (offset >= data.size())
            return false;

        std::uint8_t marker = data[offset++];

        // End Of Image
        if (marker == 0xD9)
        {
            return offset == data.size();
        }

        // Start Of Scan
        if (marker == 0xDA)
        {
            // Need two bytes for segment length
            if (offset + 1 >= data.size())
                return false;

            std::uint16_t segmentLength =
                (static_cast<std::uint16_t>(data[offset]) << 8) |
                data[offset + 1];

            // Length must be at least 2
            if (segmentLength < 2)
                return false;

            // Segment must fit inside the file
            if (offset + segmentLength > data.size())
                return false;

            // Skip the SOS segment
            offset += segmentLength;

            // Scan compressed image data
            while (offset + 1 < data.size())
            {
                if (data[offset] == 0xFF)
                {
                    // FF 00 means escaped FF data byte
                    if (data[offset + 1] == 0x00)
                    {
                        offset += 2;
                        continue;
                    }

                    // JPEG End Of Image
                    if (data[offset + 1] == 0xD9)
                    {
                        return offset + 2 == data.size();
                    }

                    // Another marker found
                    break;
                }

                ++offset;
            }

            continue;
        }

        // Standalone JPEG markers
        if (marker == 0x01 ||
            (marker >= 0xD0 && marker <= 0xD7))
        {
            continue;
        }

        // Other JPEG segments contain a 2-byte length
        if (offset + 1 >= data.size())
            return false;

        std::uint16_t segmentLength =
            (static_cast<std::uint16_t>(data[offset]) << 8) |
            data[offset + 1];

        if (segmentLength < 2)
            return false;

        if (offset + segmentLength > data.size())
            return false;

        offset += segmentLength;
    }

    return false;
}

struct JpegErrorManager
{
    jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

void jpegErrorExit(j_common_ptr cinfo)
{
    JpegErrorManager *errorManager =
        reinterpret_cast<JpegErrorManager *>(cinfo->err);

    longjmp(errorManager->setjmp_buffer, 1);
}

bool EvidenceValidator::validateDecodability(
    const std::string &filePath) const
{
    FILE *file = std::fopen(filePath.c_str(), "rb");

    if (!file)
        return false;

    jpeg_decompress_struct cinfo{};
    JpegErrorManager errorManager{};

    cinfo.err = jpeg_std_error(&errorManager.pub);
    errorManager.pub.error_exit = jpegErrorExit;

    if (setjmp(errorManager.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        std::fclose(file);
        return false;
    }

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, file);

    int headerResult =
        jpeg_read_header(&cinfo, TRUE);

    if (headerResult != JPEG_HEADER_OK)
    {
        jpeg_destroy_decompress(&cinfo);
        std::fclose(file);
        return false;
    }

    if (!jpeg_start_decompress(&cinfo))
    {
        jpeg_destroy_decompress(&cinfo);
        std::fclose(file);
        return false;
    }

    std::vector<JSAMPLE> buffer(
        static_cast<std::size_t>(
            cinfo.output_width *
            cinfo.output_components));

    while (cinfo.output_scanline < cinfo.output_height)
    {
        JSAMPROW rowPointer = buffer.data();

        if (jpeg_read_scanlines(
                &cinfo,
                &rowPointer,
                1) != 1)
        {
            jpeg_abort_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
            std::fclose(file);
            return false;
        }
    }

    bool success =
        jpeg_finish_decompress(&cinfo) == TRUE;

    jpeg_destroy_decompress(&cinfo);
    std::fclose(file);

    return success;
}