#pragma once

#include <filesystem>
#include <string>

#include "AuditLogger.h"
#include "CertificateGenerator.h"
#include "OperationLogger.h"
#include "SanitizationCertificate.h"
#include "SanitizationEngine.h"
#include "SanitizationEvent.h"
#include "SanitizationResult.h"
#include "../../safety/include/SafetyEngine.h"

class StorageDevice;

namespace SecureWipe
{
    struct SanitizationPipelineResult
    {
        SanitizationResult sanitization;

        bool certificateGenerated = false;
        SanitizationCertificate certificate;

        bool certificatePersisted = false;
        bool auditTrailPersisted = false;

        std::string certificatePath;
        std::string auditLogPath;
        std::string operationLogPath;

        std::string pipelineMessage;

        bool isSuccess() const
        {
            return sanitization.isSuccess() &&
                   certificateGenerated &&
                   certificatePersisted &&
                   auditTrailPersisted;
        }
    };
}

class SanitizationPipeline
{
public:
    explicit SanitizationPipeline(const std::filesystem::path& evidenceDirectory = {});

    SecureWipe::SanitizationPipelineResult execute(
        const StorageDevice& device,
        const std::string& requestId = {},
        const std::string& actorId = {},
        const std::string& workstationId = {},
        const std::string& expectedSerialNumber = {});

    const std::filesystem::path& evidenceDirectory() const;

private:
    std::filesystem::path evidenceDirectory_;
    std::filesystem::path certificateDirectory_;
    std::filesystem::path auditLogPath_;
    std::filesystem::path operationLogPath_;

    SafetyEngine safetyEngine_;
    SanitizationEngine sanitizationEngine_;
    CertificateGenerator certificateGenerator_;

    AuditLogger auditLogger_;
    OperationLogger operationLogger_;

    bool appendAudit(
        SecureWipe::SanitizationAuditEvent eventType,
        SecureWipe::AuditSeverity severity,
        const SecureWipe::SanitizationResult& result,
        const std::string& requestId,
        const std::string& actorId,
        const std::string& message,
        std::string& errorMessage,
        const SafetyResult* safetyResult = nullptr);

    bool persistCertificate(
        const SecureWipe::SanitizationCertificate& certificate,
        const std::filesystem::path& path,
        std::string& errorMessage) const;
};