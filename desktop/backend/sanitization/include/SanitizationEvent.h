#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "SanitizationMethod.h"
#include "SanitizationResult.h"
#include "SanitizationCertificate.h"
#include "../../safety/include/SafetyResult.h"

namespace SecureWipe
{
    enum class AuditSeverity
    {
        INFO,
        WARNING,
        FAILURE
    };

    enum class SanitizationAuditEvent
    {
        PIPELINE_STARTED,
        TARGET_VALIDATED,
        SAFETY_CHECK_COMPLETED,
        METHOD_SELECTED,
        SANITIZATION_STARTED,
        SANITIZATION_COMPLETED,
        VERIFICATION_COMPLETED,
        CERTIFICATE_GENERATED,
        CERTIFICATE_PERSISTED,
        PIPELINE_COMPLETED,
        PIPELINE_FAILED
    };

    struct SanitizationEvent
    {
        std::string eventId;
        std::string timestampUtc;

        SanitizationAuditEvent eventType =
            SanitizationAuditEvent::PIPELINE_STARTED;

        AuditSeverity severity =
            AuditSeverity::INFO;

        std::string operationId;
        std::string requestId;
        std::string actorId;

        std::string deviceId;
        std::string model;
        std::string serialNumber;
        std::string interfaceType;

        std::uint64_t capacityBytes = 0;

        SanitizationMethod method =
            SanitizationMethod::Unsupported;

        SanitizationStatus sanitizationStatus =
            SanitizationStatus::NOT_STARTED;

        VerificationStatus verificationStatus =
            VerificationStatus::NOT_PERFORMED;

        std::uint64_t bytesProcessed = 0;
        std::uint64_t bytesVerified = 0;
        std::uint32_t verificationSamples = 0;

        SanitizationErrorCode error =
            SanitizationErrorCode::NONE;

        std::uint32_t nativeErrorCode = 0;

        std::string safetyDecision;
        std::string safetySummary;

        std::vector<SafetyCheckResult> safetyChecks;

        std::string message;

        std::string previousEventHash;
        std::string eventHash;

        static const char* eventTypeName(
            SanitizationAuditEvent eventType);

        static const char* severityName(
            AuditSeverity severity);

        static const char* methodName(
            SanitizationMethod method);

        static const char* sanitizationStatusName(
            SanitizationStatus status);

        static const char* verificationStatusName(
            VerificationStatus status);

        static const char* errorCodeName(
            SanitizationErrorCode error);

        static std::string generateEventId();

        static std::string generateTimestampUtc();

        std::string canonicalData() const;

        std::string toJsonLine() const;
    };
}