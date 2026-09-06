#pragma once

#include <cstdint>
#include <string>

#include "SanitizationMethod.h"
#include "SanitizationResult.h"

namespace SecureWipe
{
    struct SanitizationCertificate
    {
        // Certificate identity
        std::string certificateId;
        std::string operationId;
        std::string requestId;

        // Device identity
        std::string deviceId;
        std::string model;
        std::string serialNumber;
        std::uint64_t capacityBytes = 0;
        std::string interfaceType;

        // Sanitization information
        SanitizationMethod method = SanitizationMethod::Unsupported;
        SanitizationStatus status = SanitizationStatus::NOT_STARTED;
        std::uint64_t bytesProcessed = 0;
        std::uint64_t operationDurationMs = 0;

        // Verification evidence
        VerificationStatus verificationStatus = VerificationStatus::NOT_PERFORMED;
        bool verificationPerformed = false;
        bool verificationPassed = false;
        std::uint64_t bytesVerified = 0;
        std::uint32_t verificationSamples = 0;
        bool deviceReportedSuccess = false;
        bool globalDataErased = false;
        std::uint32_t nativeErrorCode = 0;
        std::string verificationMessage;

        // Certificate metadata
        std::string generatedAt;
        std::string hashAlgorithm = "SHA-256";
        std::string certificateHash;

        // Final human-readable message
        std::string message;

        bool isValid() const
        {
            return !certificateId.empty() &&
                   !operationId.empty() &&
                   !deviceId.empty() &&
                   !serialNumber.empty() &&
                   capacityBytes > 0 &&
                   status == SanitizationStatus::COMPLETED &&
                   verificationStatus == VerificationStatus::PASSED &&
                   verificationPerformed &&
                   verificationPassed;
        }
    };
}