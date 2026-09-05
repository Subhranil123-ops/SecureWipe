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

    VerificationMethod method =
        VerificationMethod::NONE;

    std::uint64_t bytesVerified = 0;
    std::uint32_t samples = 0;

    std::uint32_t nativeStatusCode = 0;

    bool deviceReportedSuccess = false;
    bool globalDataErased = false;

    std::string message;
};