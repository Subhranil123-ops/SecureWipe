#pragma once

#include <Windows.h>

#include "EvidenceItem.h"

#include <cstdint>
#include <string>
#include <vector>

struct EvidenceCollectionSummary
{
    bool sourceOpened = false;
    bool completed = false;

    std::uint64_t bytesScanned = 0;
    std::uint64_t candidatesFound = 0;
    std::uint64_t recoveredArtifacts = 0;
    std::uint64_t validatedArtifacts = 0;
    std::uint64_t rejectedArtifacts = 0;
    std::uint64_t highConfidenceArtifacts = 0;
    std::uint64_t recoveredBytes = 0;
};

struct EvidenceCollectionResult
{
    std::vector<EvidenceItem> evidence;

    EvidenceCollectionSummary summary;
};

class EvidenceCollector
{
private:
    std::string detectFileType(
        const std::vector<std::uint8_t> &buffer,
        std::size_t offset
    ) const;

    std::size_t findEndOffset(
        const std::vector<std::uint8_t> &buffer,
        std::size_t startOffset
    ) const;

    bool readChunk(
        HANDLE deviceHandle,
        std::vector<std::uint8_t> &buffer,
        bool &readError
    ) const;

    bool carveArtifacts(
        const std::vector<std::uint8_t> &buffer,
        std::size_t startOffset,
        std::size_t endOffset,
        const std::string &outputPath
    ) const;

    std::uint64_t findArtifactEnd(
        const std::vector<std::uint8_t> &buffer,
        std::size_t offset,
        const std::string &fileType
    ) const;

    std::string generateArtifactId(
        std::uint64_t index
    ) const;

public:
    std::vector<EvidenceItem> collect(
        const std::string &source
    );

    EvidenceCollectionResult collectWithSummary(
        const std::string &source
    );
};