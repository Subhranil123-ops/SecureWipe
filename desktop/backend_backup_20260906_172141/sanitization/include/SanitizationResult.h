#pragma once

#include <cstdint>
#include <string>

#include "SanitizationMethod.h"

namespace SecureWipe
{
    enum class SanitizationStatus
    {
        NOT_STARTED,
        IN_PROGRESS,
        COMPLETED,
        FAILED,
        ABORTED
    };

    enum class VerificationStatus
    {
        NOT_PERFORMED,
        IN_PROGRESS,
        PASSED,
        FAILED
    };

    enum class SanitizationErrorCode
    {
        NONE,

        // Validation / Safety
        SAFETY_VALIDATION_FAILED,
        MISSING_DEVICE_ID,
        UNKNOWN_DEVICE_CAPACITY,

        // Device / I/O
        DEVICE_OPEN_FAILED,

        // Sanitization capability / method
        UNSUPPORTED_SANITIZATION_METHOD,
        NVME_ALGORITHM_UNAVAILABLE,
        ATA_ALGORITHM_UNAVAILABLE,

        // Sanitization execution
        SANITIZATION_EXECUTION_FAILED,

        // Verification
        VERIFICATION_FAILED
    };

    struct SanitizationResult
    {
        // Operation identity
        std::string operationId;

        // Overall operation status
        SanitizationStatus status =
            SanitizationStatus::NOT_STARTED;

        // Target device identity
        std::string deviceId;
        std::string model;
        std::string serialNumber;
        std::string interfaceType;
        std::uint64_t capacityBytes = 0;

        // Selected sanitization method
        SanitizationMethod method =
            SanitizationMethod::Unsupported;

        // Execution information
        std::uint64_t bytesProcessed = 0;
        std::uint64_t operationDurationMs = 0;

        // Verification information
        VerificationStatus verificationStatus =
            VerificationStatus::NOT_PERFORMED;

        bool verificationPerformed = false;

        std::uint64_t bytesVerified = 0;
        std::uint32_t verificationSamples = 0;

        std::string verificationMessage;

        // Error information
        SanitizationErrorCode error =
            SanitizationErrorCode::NONE;

        std::uint32_t nativeErrorCode = 0;

        std::string errorMessage;

        // Human-readable summary
        std::string message;

        bool isSuccess() const
        {
            return status == SanitizationStatus::COMPLETED &&
                   verificationStatus == VerificationStatus::PASSED;
        }
    };
}