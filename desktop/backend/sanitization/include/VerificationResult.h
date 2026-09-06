#pragma once

#include <cstdint>
#include <string>

enum class VerificationMethod
{
    NONE,
    HOST_READ_BACK,
    NVME_SANITIZE_STATUS,
    ATA_SANITIZE_STATUS
};

struct VerificationResult
{
    bool performed = false;
    bool passed = false;
    bool sanitizationCompleted = false;

    VerificationMethod method = VerificationMethod::NONE;

    std::uint64_t bytesVerified = 0;
    std::uint32_t samples = 0;

    std::uint32_t nativeErrorCode = 0;
    std::uint64_t failedOffset = 0;

    std::uint32_t requestedBytes = 0;
    std::uint32_t actualBytes = 0;

    bool deviceReportedSuccess = false;
    bool globalDataErased = false;

    std::string phase;
    std::string message;
};