#include "EvidenceCollector.h"

#include "EvidenceValidator.h"
#include "HashCalculator.h"
#include "ConfidenceScorer.h"

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::size_t EvidenceCollector::findEndOffset(
    const std::vector<std::uint8_t> &buffer,
    std::size_t startOffset) const
{
    if (startOffset >= buffer.size())
    {
        return std::string::npos;
    }

    for (std::size_t offset = startOffset + 2;
         offset + 1 < buffer.size();
         ++offset)
    {
        if (buffer[offset] == 0xFF &&
            buffer[offset + 1] == 0xD9)
        {
            return offset + 1;
        }
    }

    return std::string::npos;
}

std::string EvidenceCollector::detectFileType(
    const std::vector<std::uint8_t> &buffer,
    std::size_t offset) const
{
    if (offset + 2 < buffer.size() &&
        buffer[offset] == 0xFF &&
        buffer[offset + 1] == 0xD8 &&
        buffer[offset + 2] == 0xFF)
    {
        return "JPEG";
    }

    return "UNKNOWN";
}

bool EvidenceCollector::readChunk(
    HANDLE deviceHandle,
    std::vector<std::uint8_t> &buffer,
    bool &readError) const
{
    readError = false;

    constexpr std::size_t CHUNK_SIZE =
        4 * 1024 * 1024;

    buffer.resize(CHUNK_SIZE);

    DWORD bytesRead = 0;

    const BOOL success =
        ReadFile(
            deviceHandle,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &bytesRead,
            nullptr);

    if (!success || bytesRead == 0)
    {
        const DWORD error =
            GetLastError();

        readError =
            !success &&
            error != ERROR_HANDLE_EOF;

        buffer.clear();

        return false;
    }

    buffer.resize(bytesRead);

    std::cout
        << "Bytes read   : "
        << bytesRead
        << '\n';

    return true;
}

bool EvidenceCollector::carveArtifacts(
    const std::vector<std::uint8_t> &buffer,
    std::size_t startOffset,
    std::size_t endOffset,
    const std::string &outputPath) const
{
    if (startOffset >= buffer.size() ||
        endOffset >= buffer.size() ||
        startOffset > endOffset)
    {
        return false;
    }

    std::ofstream output(
        outputPath,
        std::ios::binary);

    if (!output)
    {
        std::cerr
            << "Unable to create recovered file.\n";

        return false;
    }

    const std::size_t artifactSize =
        endOffset - startOffset + 1;

    output.write(
        reinterpret_cast<const char *>(
            buffer.data() + startOffset),
        static_cast<std::streamsize>(
            artifactSize));

    if (!output)
    {
        std::cerr
            << "Failed to write recovered file.\n";

        return false;
    }

    std::cout
        << "Artifact carved successfully.\n";

    std::cout
        << "Recovered file: "
        << outputPath
        << '\n';

    std::cout
        << "Recovered size: "
        << artifactSize
        << " bytes\n";

    return true;
}

EvidenceCollectionResult
EvidenceCollector::collectWithSummary(
    const std::string &source)
{
    EvidenceCollectionResult result;

    auto &evidence =
        result.evidence;

    auto &summary =
        result.summary;

    EvidenceValidator validator;
    HashCalculator hashCalculator;
    ConfidenceScorer confidenceScorer;

    HANDLE deviceHandle =
        CreateFileA(
            source.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ |
                FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr);

    if (deviceHandle ==
        INVALID_HANDLE_VALUE)
    {
        const DWORD error =
            GetLastError();

        std::cerr
            << "Unable to open forensic source.\n";

        std::cerr
            << "Source: "
            << source
            << '\n';

        std::cerr
            << "Windows error: "
            << error
            << '\n';

        summary.sourceOpened =
            false;

        summary.completed =
            false;

        return result;
    }

    summary.sourceOpened =
        true;

    std::cout
        << "Forensic source opened successfully.\n";

    std::vector<std::uint8_t>
        buffer;

    std::uint64_t globalOffset =
        0;

    std::size_t chunkNumber =
        0;

    bool jpegInProgress =
        false;

    std::uint64_t jpegStartOffset =
        0;

    bool hasPreviousByte =
        false;

    std::uint8_t previousByte =
        0;

    bool hasPreviousTwoBytes =
        false;

    std::uint8_t previousTwoBytes =
        0;

    std::string currentArtifactId;

    std::string currentOutputPath;

    std::ofstream recoveredFile;

    std::filesystem::create_directories(
        "recovered");

    bool readError =
        false;

    while (
        readChunk(
            deviceHandle,
            buffer,
            readError))
    {
        ++chunkNumber;

        const std::uint64_t
            chunkStartOffset =
                globalOffset;

        std::cout
            << "\n==============================\n";

        std::cout
            << "Chunk: "
            << chunkNumber
            << '\n';

        std::cout
            << "Chunk start offset: "
            << chunkStartOffset
            << '\n';

        std::cout
            << "Chunk size: "
            << buffer.size()
            << " bytes\n";

        std::size_t scanOffset =
            0;

        while (
            scanOffset <
            buffer.size())
        {
            if (!jpegInProgress)
            {
                bool boundaryJpeg =
                    false;

                std::size_t boundaryBytes =
                    0;

                if (
                    scanOffset == 0 &&
                    hasPreviousTwoBytes &&
                    previousTwoBytes == 0xFF &&
                    previousByte == 0xD8 &&
                    buffer.size() >= 1 &&
                    buffer[0] == 0xFF)
                {
                    boundaryJpeg =
                        true;

                    boundaryBytes =
                        2;
                }
                else if (
                    scanOffset == 0 &&
                    hasPreviousByte &&
                    previousByte == 0xFF &&
                    buffer.size() >= 2 &&
                    buffer[0] == 0xD8 &&
                    buffer[1] == 0xFF)
                {
                    boundaryJpeg =
                        true;

                    boundaryBytes =
                        1;
                }

                const std::string type =
                    detectFileType(
                        buffer,
                        scanOffset);

                if (
                    type == "JPEG" ||
                    boundaryJpeg)
                {
                    const std::uint64_t
                        actualStartOffset =
                            chunkStartOffset +
                            scanOffset -
                            boundaryBytes;

                    ++summary.candidatesFound;

                    std::cout
                        << "Found JPEG at global offset: "
                        << actualStartOffset
                        << '\n';

                    const std::uint64_t
                        candidateNumber =
                            summary.candidatesFound;

                    currentArtifactId =
                        "artifact_" +
                        std::to_string(
                            candidateNumber);

                    currentOutputPath =
                        (std::filesystem::absolute(
                             std::filesystem::path(
                                 "recovered") /
                             ("recovered_" +
                              std::to_string(
                                  candidateNumber) +
                              ".jpg")))
                            .string();

                    jpegStartOffset =
                        actualStartOffset;

                    jpegInProgress =
                        true;

                    recoveredFile.open(
                        currentOutputPath,
                        std::ios::binary);

                    if (!recoveredFile)
                    {
                        std::cerr
                            << "Unable to create recovered file.\n";

                        jpegInProgress =
                            false;

                        ++scanOffset;

                        continue;
                    }

                    if (boundaryBytes == 2)
                    {
                        const std::uint8_t
                            signatureBytes[2] =
                                {
                                    previousTwoBytes,
                                    previousByte};

                        recoveredFile.write(
                            reinterpret_cast<
                                const char *>(signatureBytes),
                            2);
                    }
                    else if (
                        boundaryBytes == 1)
                    {
                        recoveredFile.write(
                            reinterpret_cast<
                                const char *>(&previousByte),
                            1);
                    }

                    std::cout
                        << "JPEG carving started.\n";
                }
            }

            if (jpegInProgress)
            {
                bool endFound =
                    false;

                if (
                    scanOffset == 0 &&
                    hasPreviousByte &&
                    previousByte == 0xFF &&
                    buffer[0] == 0xD9)
                {
                    recoveredFile.write(
                        reinterpret_cast<
                            const char *>(&buffer[0]),
                        1);

                    if (!recoveredFile)
                    {
                        std::cerr
                            << "Failed to write recovered JPEG.\n";

                        recoveredFile.close();

                        jpegInProgress =
                            false;

                        ++scanOffset;

                        continue;
                    }

                    const std::uint64_t
                        actualEndOffset =
                            chunkStartOffset;

                    const std::uint64_t
                        artifactSize =
                            actualEndOffset -
                            jpegStartOffset +
                            1;

                    ++summary.recoveredArtifacts;

                    summary.recoveredBytes +=
                        artifactSize;

                    recoveredFile.close();

                    EvidenceItem item;

                    item.artifactId =
                        currentArtifactId;

                    item.source =
                        source;

                    item.offset =
                        jpegStartOffset;

                    item.size =
                        artifactSize;

                    item.fileType =
                        "JPEG";

                    item.fileName =
                        std::filesystem::path(
                            currentOutputPath)
                            .filename()
                            .string();

                    item.recoveredPath =
                        currentOutputPath;

                    item.recovered =
                        true;

                    item.headerValid =
                        validator.validateHeader(
                            item.recoveredPath);

                    item.footerValid =
                        validator.validateFooter(
                            item.recoveredPath);

                    item.sizeValid =
                        validator.validateSize(
                            item);

                    item.structureValid =
                        validator.validateStructure(
                            item.recoveredPath);

                    item.decodable =
                        validator.validateDecodability(
                            item.recoveredPath);

                    item.validated =
                        item.headerValid &&
                        item.footerValid &&
                        item.sizeValid &&
                        item.structureValid &&
                        item.decodable;

                    if (item.validated)
                    {
                        ++summary.validatedArtifacts;

                        if (
                            hashCalculator.calculateSha256(
                                item.recoveredPath,
                                item.sha256))
                        {
                            confidenceScorer.calculate(
                                item);

                            if (
                                item.confidenceScore >=
                                80)
                            {
                                ++summary.highConfidenceArtifacts;
                            }

                            evidence.push_back(
                                item);
                        }
                        else
                        {
                            ++summary.rejectedArtifacts;
                        }
                    }
                    else
                    {
                        ++summary.rejectedArtifacts;
                    }

                    jpegInProgress =
                        false;

                    scanOffset =
                        1;

                    continue;
                }

                for (
                    std::size_t offset =
                        scanOffset;
                    offset + 1 <
                    buffer.size();
                    ++offset)
                {
                    if (
                        buffer[offset] == 0xFF &&
                        buffer[offset + 1] == 0xD9)
                    {
                        const std::size_t
                            endOffset =
                                offset + 1;

                        recoveredFile.write(
                            reinterpret_cast<
                                const char *>(
                                buffer.data() +
                                scanOffset),
                            static_cast<
                                std::streamsize>(
                                endOffset -
                                scanOffset +
                                1));

                        if (!recoveredFile)
                        {
                            std::cerr
                                << "Failed to write recovered JPEG.\n";

                            recoveredFile.close();

                            jpegInProgress =
                                false;

                            endFound =
                                true;

                            break;
                        }

                        const std::uint64_t
                            actualEndOffset =
                                chunkStartOffset +
                                endOffset;

                        const std::uint64_t
                            artifactSize =
                                actualEndOffset -
                                jpegStartOffset +
                                1;

                        ++summary.recoveredArtifacts;

                        summary.recoveredBytes +=
                            artifactSize;

                        recoveredFile.close();

                        EvidenceItem item;

                        item.artifactId =
                            currentArtifactId;

                        item.source =
                            source;

                        item.offset =
                            jpegStartOffset;

                        item.size =
                            artifactSize;

                        item.fileType =
                            "JPEG";

                        item.fileName =
                            std::filesystem::path(
                                currentOutputPath)
                                .filename()
                                .string();

                        item.recoveredPath =
                            currentOutputPath;

                        item.recovered =
                            true;

                        item.headerValid =
                            validator.validateHeader(
                                item.recoveredPath);

                        item.footerValid =
                            validator.validateFooter(
                                item.recoveredPath);

                        item.sizeValid =
                            validator.validateSize(
                                item);

                        item.structureValid =
                            validator.validateStructure(
                                item.recoveredPath);

                        item.decodable =
                            validator.validateDecodability(
                                item.recoveredPath);

                        item.validated =
                            item.headerValid &&
                            item.footerValid &&
                            item.sizeValid &&
                            item.structureValid &&
                            item.decodable;

                        if (item.validated)
                        {
                            ++summary.validatedArtifacts;

                            if (
                                hashCalculator.calculateSha256(
                                    item.recoveredPath,
                                    item.sha256))
                            {
                                confidenceScorer.calculate(
                                    item);

                                if (
                                    item.confidenceScore >=
                                    80)
                                {
                                    ++summary.highConfidenceArtifacts;
                                }

                                evidence.push_back(
                                    item);
                            }
                            else
                            {
                                ++summary.rejectedArtifacts;
                            }
                        }
                        else
                        {
                            ++summary.rejectedArtifacts;
                        }

                        jpegInProgress =
                            false;

                        scanOffset =
                            endOffset + 1;

                        endFound =
                            true;

                        break;
                    }
                }

                if (endFound)
                {
                    continue;
                }

                recoveredFile.write(
                    reinterpret_cast<
                        const char *>(
                        buffer.data() +
                        scanOffset),
                    static_cast<
                        std::streamsize>(
                        buffer.size() -
                        scanOffset));

                if (!recoveredFile)
                {
                    std::cerr
                        << "Failed to write JPEG chunk.\n";

                    recoveredFile.close();

                    jpegInProgress =
                        false;

                    break;
                }

                scanOffset =
                    buffer.size();

                continue;
            }

            ++scanOffset;
        }

        if (!buffer.empty())
        {
            if (buffer.size() >= 2)
            {
                previousTwoBytes =
                    buffer[buffer.size() - 2];

                previousByte =
                    buffer[buffer.size() - 1];

                hasPreviousTwoBytes =
                    true;

                hasPreviousByte =
                    true;
            }
            else
            {
                previousByte =
                    buffer.back();

                hasPreviousByte =
                    true;

                hasPreviousTwoBytes =
                    false;
            }
        }

        globalOffset +=
            buffer.size();
    }

    if (jpegInProgress)
    {
        std::cout
            << "JPEG started at global offset "
            << jpegStartOffset
            << " but end marker was not found before EOF.\n";

        if (recoveredFile.is_open())
        {
            recoveredFile.close();
        }
    }

    CloseHandle(
        deviceHandle);

    summary.bytesScanned =
        globalOffset;

    summary.completed =
        !readError;

    std::cout
        << "\n==============================\n";

    std::cout
        << (summary.completed
                ? "Forensic source scanning completed.\n"
                : "Forensic source scanning stopped because of a read error.\n");

    std::cout
        << "Total bytes scanned: "
        << summary.bytesScanned
        << '\n';

    std::cout
        << "Candidates found: "
        << summary.candidatesFound
        << '\n';

    std::cout
        << "Recovered artifacts: "
        << summary.recoveredArtifacts
        << '\n';

    std::cout
        << "Validated artifacts: "
        << summary.validatedArtifacts
        << '\n';

    std::cout
        << "Rejected artifacts: "
        << summary.rejectedArtifacts
        << '\n';

    std::cout
        << "High confidence: "
        << summary.highConfidenceArtifacts
        << '\n';

    return result;
}

std::vector<EvidenceItem>
EvidenceCollector::collect(const std::string &source)
{
    return collectWithSummary(source)
        .evidence;
}