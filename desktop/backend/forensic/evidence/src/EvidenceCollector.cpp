#include "EvidenceCollector.h"

#include "EvidenceValidator.h"
#include "HashCalculator.h"
#include "ConfidenceScorer.h"

#include <Windows.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
    /*
     * Larger sequential reads dramatically reduce the number of
     * ReadFile() calls for large physical disks.
     *
     * 16 MiB is deliberately chosen instead of an extremely large
     * buffer so memory usage remains reasonable.
     */
    constexpr std::size_t FORENSIC_CHUNK_SIZE =
        16ULL * 1024ULL * 1024ULL;

    constexpr std::uint8_t JPEG_MARKER_PREFIX =
        0xFF;

    constexpr std::uint8_t JPEG_START_SECOND =
        0xD8;

    constexpr std::uint8_t JPEG_END_SECOND =
        0xD9;

    /*
     * Find the next 0xFF byte.
     *
     * std::memchr() is generally much faster than a manual byte-by-byte
     * loop because the C runtime can use optimized memory operations.
     */
    const std::uint8_t *findMarkerPrefix(
        const std::vector<std::uint8_t> &buffer,
        std::size_t startOffset)
    {
        if (startOffset >= buffer.size())
        {
            return nullptr;
        }

        const std::size_t remaining =
            buffer.size() - startOffset;

        return static_cast<const std::uint8_t *>(
            std::memchr(
                buffer.data() + startOffset,
                JPEG_MARKER_PREFIX,
                remaining));
    }

    /*
     * Find a JPEG SOI marker:
     *
     * FF D8 FF
     *
     * Returns the offset of FF.
     */
    std::size_t findJpegStart(
        const std::vector<std::uint8_t> &buffer,
        std::size_t startOffset)
    {
        while (startOffset < buffer.size())
        {
            const std::uint8_t *marker =
                findMarkerPrefix(
                    buffer,
                    startOffset);

            if (marker == nullptr)
            {
                return std::string::npos;
            }

            const std::size_t offset =
                static_cast<std::size_t>(
                    marker - buffer.data());

            /*
             * Need three bytes:
             *
             * FF D8 FF
             */
            if (offset + 2 < buffer.size() &&
                buffer[offset + 1] ==
                    JPEG_START_SECOND &&
                buffer[offset + 2] ==
                    JPEG_MARKER_PREFIX)
            {
                return offset;
            }

            /*
             * This FF was not a JPEG SOI marker.
             * Jump directly to the byte after FF.
             */
            startOffset =
                offset + 1;
        }

        return std::string::npos;
    }

    /*
     * Find JPEG EOI marker:
     *
     * FF D9
     *
     * Returns the offset of D9.
     */
    std::size_t findJpegEnd(
        const std::vector<std::uint8_t> &buffer,
        std::size_t startOffset)
    {
        while (startOffset < buffer.size())
        {
            const std::uint8_t *marker =
                findMarkerPrefix(
                    buffer,
                    startOffset);

            if (marker == nullptr)
            {
                return std::string::npos;
            }

            const std::size_t offset =
                static_cast<std::size_t>(
                    marker - buffer.data());

            if (offset + 1 < buffer.size() &&
                buffer[offset + 1] ==
                    JPEG_END_SECOND)
            {
                return offset + 1;
            }

            startOffset =
                offset + 1;
        }

        return std::string::npos;
    }
}

std::size_t EvidenceCollector::findEndOffset(
    const std::vector<std::uint8_t> &buffer,
    std::size_t startOffset) const
{
    if (startOffset >= buffer.size())
    {
        return std::string::npos;
    }

    return findJpegEnd(
        buffer,
        startOffset);
}

std::string EvidenceCollector::detectFileType(
    const std::vector<std::uint8_t> &buffer,
    std::size_t offset) const
{
    if (offset + 2 >= buffer.size())
    {
        return "UNKNOWN";
    }

    /*
     * JPEG SOI:
     *
     * FF D8 FF
     */
    if (buffer[offset] ==
            JPEG_MARKER_PREFIX &&
        buffer[offset + 1] ==
            JPEG_START_SECOND &&
        buffer[offset + 2] ==
            JPEG_MARKER_PREFIX)
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

    /*
     * Reuse the vector capacity between iterations.
     *
     * resize() does not normally reallocate after the first iteration
     * because the vector already owns enough capacity.
     */
    buffer.resize(
        FORENSIC_CHUNK_SIZE);

    DWORD bytesRead = 0;

    const BOOL success =
        ReadFile(
            deviceHandle,
            buffer.data(),
            static_cast<DWORD>(
                buffer.size()),
            &bytesRead,
            nullptr);

    if (!success)
    {
        const DWORD error =
            GetLastError();

        /*
         * ERROR_HANDLE_EOF is a normal end condition.
         * Any other error means the scan really failed.
         */
        readError =
            error != ERROR_HANDLE_EOF;

        buffer.clear();

        return false;
    }

    if (bytesRead == 0)
    {
        buffer.clear();

        return false;
    }

    buffer.resize(
        static_cast<std::size_t>(
            bytesRead));

    /*
     * Deliberately no per-chunk std::cout here.
     *
     * Console output for thousands of chunks can become a surprisingly
     * expensive bottleneck during multi-GB forensic scans.
     */

    return true;
}

bool EvidenceCollector::carveArtifacts(
    const std::vector<std::uint8_t> &buffer,
    std::size_t startOffset,
    std::size_t endOffset,
    const std::string &outputPath) const
{
    if (buffer.empty() ||
        startOffset >= buffer.size() ||
        endOffset >= buffer.size() ||
        startOffset > endOffset)
    {
        return false;
    }

    std::ofstream output(
        outputPath,
        std::ios::binary |
        std::ios::out |
        std::ios::trunc);

    if (!output)
    {
        return false;
    }

    const std::size_t artifactSize =
        endOffset -
        startOffset +
        1;

    output.write(
        reinterpret_cast<const char *>(
            buffer.data() +
            startOffset),
        static_cast<std::streamsize>(
            artifactSize));

    return static_cast<bool>(
        output);
}

EvidenceCollectionResult
EvidenceCollector::collectWithSummary(
    const std::string &source,
    const EvidenceProgressCallback &progressCallback)
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
        summary.sourceOpened =
            false;

        summary.completed =
            false;

        return result;
    }

    summary.sourceOpened =
        true;

    LARGE_INTEGER sourceSize{};

    if (GetFileSizeEx(
            deviceHandle,
            &sourceSize) &&
        sourceSize.QuadPart >= 0)
    {
        summary.totalBytes =
            static_cast<std::uint64_t>(
                sourceSize.QuadPart);
    }

    if (progressCallback)
    {
        progressCallback(
            0,
            summary.totalBytes);
    }

    /*
     * Reserve some space for evidence metadata.
     *
     * This does not affect correctness. It simply reduces vector
     * reallocations when many artifacts are recovered.
     */
    evidence.reserve(64);

    std::vector<std::uint8_t>
        buffer;

    /*
     * Allocate once and reuse.
     */
    buffer.reserve(
        FORENSIC_CHUNK_SIZE);

    std::uint64_t globalOffset =
        0;

    /*
     * State used for JPEG markers crossing chunk boundaries.
     */
    bool hasPreviousByte =
        false;

    std::uint8_t previousByte =
        0;

    bool hasPreviousTwoBytes =
        false;

    std::uint8_t previousTwoBytes =
        0;

    bool jpegInProgress =
        false;

    std::uint64_t jpegStartOffset =
        0;

    std::string currentArtifactId;
    std::string currentOutputPath;

    std::ofstream recoveredFile;

    std::error_code directoryError;

    std::filesystem::create_directories(
        "recovered",
        directoryError);

    /*
     * If directory creation failed, the actual file open below will
     * fail and the candidate will simply be rejected.
     */

    bool readError =
        false;

    /*
     * ---------------------------------------------------------------
     * FINALIZE CURRENT JPEG
     * ---------------------------------------------------------------
     *
     * Both normal same-chunk EOI and cross-chunk EOI eventually reach
     * this same validation path.
     */
    auto finalizeRecoveredArtifact =
        [&](std::uint64_t actualEndOffset) -> bool
        {
            if (recoveredFile.is_open())
            {
                recoveredFile.flush();
                recoveredFile.close();
            }

            if (actualEndOffset <
                jpegStartOffset)
            {
                jpegInProgress =
                    false;

                return false;
            }

            const std::uint64_t artifactSize =
                actualEndOffset -
                jpegStartOffset +
                1;

            ++summary.recoveredArtifacts;

            summary.recoveredBytes +=
                artifactSize;

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

            /*
             * Keep the existing validation behavior.
             *
             * These calls are intentionally preserved because they
             * are part of the forensic evidence acceptance criteria.
             */
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

            if (!item.validated)
            {
                ++summary.rejectedArtifacts;

                jpegInProgress =
                    false;

                return false;
            }

            ++summary.validatedArtifacts;

            /*
             * Hash only validated artifacts.
             * This avoids hashing rejected JPEGs.
             */
            if (!hashCalculator.calculateSha256(
                    item.recoveredPath,
                    item.sha256))
            {
                ++summary.rejectedArtifacts;

                jpegInProgress =
                    false;

                return false;
            }

            confidenceScorer.calculate(
                item);

            if (item.confidenceScore >= 80)
            {
                ++summary.highConfidenceArtifacts;
            }

            /*
             * Move/copy the completed evidence item into the result
             * vector without an unnecessary intermediate object.
             */
            evidence.emplace_back(
                std::move(item));

            jpegInProgress =
                false;

            return true;
        };

    while (
        readChunk(
            deviceHandle,
            buffer,
            readError))
    {
        const std::uint64_t
            chunkStartOffset =
                globalOffset;

        const std::size_t
            bufferSize =
                buffer.size();

        if (bufferSize == 0)
        {
            continue;
        }

        std::size_t scanOffset =
            0;

        /*
         * -----------------------------------------------------------
         * MAIN CHUNK SCAN
         * -----------------------------------------------------------
         */
        while (scanOffset <
               bufferSize)
        {
            /*
             * -------------------------------------------------------
             * CASE 1:
             *
             * We are already inside a JPEG started in an earlier
             * chunk.
             * -------------------------------------------------------
             */
            if (jpegInProgress)
            {
                /*
                 * Special cross-chunk EOI:
                 *
                 * previous byte = FF
                 * current first byte = D9
                 */
                if (scanOffset == 0 &&
                    hasPreviousByte &&
                    previousByte ==
                        JPEG_MARKER_PREFIX &&
                    bufferSize >= 1 &&
                    buffer[0] ==
                        JPEG_END_SECOND)
                {
                    recoveredFile.write(
                        reinterpret_cast<
                            const char *>(
                            buffer.data()),
                        1);

                    if (!recoveredFile)
                    {
                        recoveredFile.close();

                        jpegInProgress =
                            false;

                        break;
                    }

                    const std::uint64_t
                        actualEndOffset =
                            chunkStartOffset;

                    finalizeRecoveredArtifact(
                        actualEndOffset);

                    scanOffset =
                        1;

                    continue;
                }

                /*
                 * Find FF using optimized memory search.
                 */
                const std::size_t
                    endOffset =
                    findJpegEnd(
                        buffer,
                        scanOffset);

                if (endOffset !=
                    std::string::npos)
                {
                    /*
                     * Write everything from the current scan position
                     * through D9 in one operation.
                     */
                    const std::size_t bytesToWrite =
                        endOffset -
                        scanOffset +
                        1;

                    recoveredFile.write(
                        reinterpret_cast<
                            const char *>(
                            buffer.data() +
                            scanOffset),
                        static_cast<
                            std::streamsize>(
                            bytesToWrite));

                    if (!recoveredFile)
                    {
                        recoveredFile.close();

                        jpegInProgress =
                            false;

                        break;
                    }

                    const std::uint64_t
                        actualEndOffset =
                            chunkStartOffset +
                            endOffset;

                    finalizeRecoveredArtifact(
                        actualEndOffset);

                    scanOffset =
                        endOffset + 1;

                    continue;
                }

                /*
                 * No EOI in this chunk.
                 *
                 * Write the remaining chunk in one operation.
                 */
                recoveredFile.write(
                    reinterpret_cast<
                        const char *>(
                        buffer.data() +
                        scanOffset),
                    static_cast<
                        std::streamsize>(
                        bufferSize -
                        scanOffset));

                if (!recoveredFile)
                {
                    recoveredFile.close();

                    jpegInProgress =
                        false;

                    break;
                }

                scanOffset =
                    bufferSize;

                continue;
            }

            /*
             * -------------------------------------------------------
             * CASE 2:
             *
             * Search for the next JPEG SOI.
             *
             * The old implementation checked every byte and called
             * detectFileType() at every position.
             *
             * This version jumps directly between 0xFF bytes.
             * -------------------------------------------------------
             */
            bool boundaryJpeg =
                false;

            std::size_t boundaryBytes =
                0;

            /*
             * JPEG SOI may cross the chunk boundary:
             *
             * previous two bytes: FF D8
             * current first byte: FF
             *
             * Therefore actual JPEG start is 2 bytes before this chunk.
             */
            if (scanOffset == 0 &&
                hasPreviousTwoBytes &&
                previousTwoBytes ==
                    JPEG_MARKER_PREFIX &&
                previousByte ==
                    JPEG_START_SECOND &&
                buffer[0] ==
                    JPEG_MARKER_PREFIX)
            {
                boundaryJpeg =
                    true;

                boundaryBytes =
                    2;
            }
            /*
             * Another possible boundary:
             *
             * previous byte: FF
             * current first two bytes: D8 FF
             */
            else if (
                scanOffset == 0 &&
                hasPreviousByte &&
                previousByte ==
                    JPEG_MARKER_PREFIX &&
                bufferSize >= 2 &&
                buffer[0] ==
                    JPEG_START_SECOND &&
                buffer[1] ==
                    JPEG_MARKER_PREFIX)
            {
                boundaryJpeg =
                    true;

                boundaryBytes =
                    1;
            }

            std::size_t jpegOffset =
                std::string::npos;

            if (!boundaryJpeg)
            {
                jpegOffset =
                    findJpegStart(
                        buffer,
                        scanOffset);
            }

            if (!boundaryJpeg &&
                jpegOffset ==
                    std::string::npos)
            {
                /*
                 * No JPEG start exists in the remainder of this chunk.
                 */
                scanOffset =
                    bufferSize;

                continue;
            }

            /*
             * If a boundary JPEG was detected, the current logical
             * start position is zero. Otherwise use jpegOffset.
             */
            const std::size_t
                actualBufferOffset =
                    boundaryJpeg
                        ? 0
                        : jpegOffset;

            const std::uint64_t
                actualStartOffset =
                    chunkStartOffset +
                    actualBufferOffset -
                    boundaryBytes;

            ++summary.candidatesFound;

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

            recoveredFile.open(
                currentOutputPath,
                std::ios::binary |
                std::ios::out |
                std::ios::trunc);

            if (!recoveredFile)
            {
                jpegInProgress =
                    false;

                /*
                 * Move past the candidate marker so that a bad output
                 * path does not repeatedly rediscover the same marker.
                 */
                scanOffset =
                    actualBufferOffset + 1;

                continue;
            }

            jpegInProgress =
                true;

            /*
             * Write the signature bytes that belong to the previous
             * chunk when the JPEG starts across the boundary.
             */
            if (boundaryBytes == 2)
            {
                const std::uint8_t
                    signatureBytes[2] =
                        {
                            previousTwoBytes,
                            previousByte};

                recoveredFile.write(
                    reinterpret_cast<
                        const char *>(
                        signatureBytes),
                    2);
            }
            else if (boundaryBytes == 1)
            {
                recoveredFile.write(
                    reinterpret_cast<
                        const char *>(
                        &previousByte),
                    1);
            }

            if (!recoveredFile)
            {
                recoveredFile.close();

                jpegInProgress =
                    false;

                scanOffset =
                    actualBufferOffset + 1;

                continue;
            }

            /*
             * Continue scanning from the actual JPEG start.
             *
             * For a normal JPEG:
             *   scanOffset = FF of FF D8 FF
             *
             * For a boundary JPEG:
             *   scanOffset = 0
             *
             * The next iteration enters the jpegInProgress branch.
             */
            scanOffset =
                actualBufferOffset;
        }

        /*
         * -----------------------------------------------------------
         * SAVE LAST TWO BYTES FOR CROSS-CHUNK MARKERS
         * -----------------------------------------------------------
         */
        if (bufferSize >= 2)
        {
            previousTwoBytes =
                buffer[bufferSize - 2];

            previousByte =
                buffer[bufferSize - 1];

            hasPreviousTwoBytes =
                true;

            hasPreviousByte =
                true;
        }
        else
        {
            previousByte =
                buffer[bufferSize - 1];

            hasPreviousByte =
                true;

            hasPreviousTwoBytes =
                false;
        }

        globalOffset +=
            static_cast<std::uint64_t>(
                bufferSize);

        /*
         * LIVE PROGRESS
         *
         * One callback per large chunk instead of per byte/file marker.
         * With a 16 MiB chunk this keeps UI overhead extremely small.
         */
        if (progressCallback)
        {
            progressCallback(
                globalOffset,
                summary.totalBytes);
        }
    }

    /*
     * ---------------------------------------------------------------
     * INCOMPLETE JPEG AT EOF
     * ---------------------------------------------------------------
     */
    if (jpegInProgress)
    {
        if (recoveredFile.is_open())
        {
            recoveredFile.close();
        }

        jpegInProgress =
            false;
    }

    CloseHandle(
        deviceHandle);

    summary.bytesScanned =
        globalOffset;

    summary.completed =
        !readError;

    /*
     * Ensure final progress reaches the exact scanned byte count.
     */
    if (progressCallback)
    {
        progressCallback(
            summary.bytesScanned,
            summary.totalBytes);
    }

    return result;
}

std::vector<EvidenceItem>
EvidenceCollector::collect(
    const std::string &source,
    const EvidenceProgressCallback &progressCallback)
{
    return collectWithSummary(
        source,
        progressCallback)
        .evidence;
}