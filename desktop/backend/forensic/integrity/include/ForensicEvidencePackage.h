#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../evidence/include/EvidenceCollector.h"

namespace SecureWipe
{
struct EvidenceCollectionSummary;
struct EvidenceItem;

namespace ForensicIntegrity
{

struct SourceMetadata
{
    std::string sourceType;
    std::string sourceIdentifier;
    std::string sourceName;

    std::string model;
    std::string serialNumber;
    std::uint64_t capacityBytes = 0;

    std::string interfaceType;
};

std::string createRunId();

bool buildEvidencePackage(
    const std::string& caseId,
    const std::string& workstationId,
    const SourceMetadata& source,
    const std::string& runId,
    const EvidenceCollectionSummary& summary,
    const std::vector<EvidenceItem>& evidence,
    std::string& json,
    std::string& errorMessage
);

} // namespace ForensicIntegrity
} // namespace SecureWipe