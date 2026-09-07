#include "SanitizationPipeline.h"

#include "CertificateGenerator.h"
#include "SanitizationCapability.h"
#include "StorageDevice.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
SecureWipe::AuditSeverity auditErrorSeverity()
{
    return static_cast<SecureWipe::AuditSeverity>(2);
}

std::filesystem::path defaultEvidenceDirectory()
{
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");

    if (localAppData != nullptr && localAppData[0] != '\0')
        return std::filesystem::path(localAppData) / "SecureWipe" / "evidence";
#endif

    const char* home = std::getenv("HOME");

    if (home != nullptr && home[0] != '\0')
        return std::filesystem::path(home) / ".securewipe" / "evidence";

    return std::filesystem::current_path() / "evidence";
}

std::string escapeJson(const std::string& value)
{
    std::ostringstream output;

    for (const char character : value)
    {
        switch (character)
        {
        case '\\': output << "\\\\"; break;
        case '"': output << "\\\""; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default: output << character; break;
        }
    }

    return output.str();
}
}

SanitizationPipeline::SanitizationPipeline(const std::filesystem::path& evidenceDirectory)
    : evidenceDirectory_(evidenceDirectory.empty() ? defaultEvidenceDirectory() : evidenceDirectory)
    , certificateDirectory_(evidenceDirectory_ / "certificates")
    , auditLogPath_(evidenceDirectory_ / "audit" / "sanitization_audit.jsonl")
    , operationLogPath_(evidenceDirectory_ / "logs" / "sanitization.log")
    , auditLogger_(auditLogPath_)
    , operationLogger_(operationLogPath_)
{
}

const std::filesystem::path& SanitizationPipeline::evidenceDirectory() const
{
    return evidenceDirectory_;
}

SecureWipe::SanitizationPipelineResult SanitizationPipeline::execute(
    const StorageDevice& device,
    const std::string& requestId,
    const std::string& actorId)
{
    SecureWipe::SanitizationPipelineResult pipelineResult;
    pipelineResult.auditLogPath = auditLogPath_.string();
    pipelineResult.operationLogPath = operationLogPath_.string();

    SecureWipe::SanitizationResult initialResult;
    initialResult.deviceId = device.getDeviceId();
    initialResult.model = device.getModel();
    initialResult.serialNumber = device.getSerialNumber();
    initialResult.interfaceType = device.getInterfaceType();
    initialResult.capacityBytes = device.getCapacityBytes();

    std::string auditError;
    std::string logError;
    bool auditOk = true;

    operationLogger_.info(
        "Host Overwrite vertical pipeline started. device=" + device.getDeviceId() +
        ", requestId=" + requestId + ", actorId=" + actorId,
        logError);

    if (!this->appendAudit(
            SecureWipe::SanitizationAuditEvent::PIPELINE_STARTED,
            SecureWipe::AuditSeverity::INFO,
            initialResult,
            requestId,
            actorId,
            "Sanitization pipeline started.",
            auditError))
    {
        auditOk = false;
        pipelineResult.sanitization = initialResult;
        pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
        pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
        pipelineResult.sanitization.message = "Unable to write initial audit event: " + auditError;
        pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;
        pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
        pipelineResult.auditTrailPersisted = false;
        return pipelineResult;
    }

    try
    {
        if (device.getDeviceId().empty())
        {
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::MISSING_DEVICE_ID;
            pipelineResult.sanitization.message = "Sanitization target has no physical device identifier.";
            pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;

            if (!this->appendAudit(
                    SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                    auditErrorSeverity(),
                    pipelineResult.sanitization,
                    requestId,
                    actorId,
                    pipelineResult.sanitization.message,
                    auditError))
                auditOk = false;

            pipelineResult.auditTrailPersisted = auditOk;
            pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
            return pipelineResult;
        }

        if (device.getCapacityBytes() == 0)
        {
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::UNKNOWN_DEVICE_CAPACITY;
            pipelineResult.sanitization.message = "Sanitization target capacity is unknown.";
            pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;

            if (!this->appendAudit(
                    SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                    auditErrorSeverity(),
                    pipelineResult.sanitization,
                    requestId,
                    actorId,
                    pipelineResult.sanitization.message,
                    auditError))
                auditOk = false;

            pipelineResult.auditTrailPersisted = auditOk;
            pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
            return pipelineResult;
        }

        safetyEngine_.setExpectedTarget(device);
        const SafetyResult safetyResult = safetyEngine_.evaluateWithResult(device);
        initialResult.message = safetyResult.summary;

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::SAFETY_CHECK_COMPLETED,
                safetyResult.isOverallSafe ? SecureWipe::AuditSeverity::INFO : auditErrorSeverity(),
                initialResult,
                requestId,
                actorId,
                safetyResult.summary,
                auditError,
                &safetyResult))
        {
            auditOk = false;
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
            pipelineResult.sanitization.message = "Audit persistence failed during safety evaluation: " + auditError;
            pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;
            pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
            pipelineResult.auditTrailPersisted = false;
            return pipelineResult;
        }

        if (!safetyResult.isOverallSafe)
        {
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SAFETY_VALIDATION_FAILED;
            pipelineResult.sanitization.message = safetyResult.summary;
            pipelineResult.sanitization.errorMessage = safetyResult.summary;

            if (!this->appendAudit(
                    SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                    auditErrorSeverity(),
                    pipelineResult.sanitization,
                    requestId,
                    actorId,
                    safetyResult.summary,
                    auditError,
                    &safetyResult))
                auditOk = false;

            operationLogger_.warning("Sanitization blocked by safety engine. " + safetyResult.summary, logError);
            pipelineResult.auditTrailPersisted = auditOk;
            pipelineResult.pipelineMessage = safetyResult.summary;
            return pipelineResult;
        }

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::TARGET_VALIDATED,
                SecureWipe::AuditSeverity::INFO,
                initialResult,
                requestId,
                actorId,
                "Target passed pre-sanitization safety validation.",
                auditError,
                &safetyResult))
        {
            auditOk = false;
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
            pipelineResult.sanitization.message = "Audit persistence failed after target validation: " + auditError;
            pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;
            pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
            pipelineResult.auditTrailPersisted = false;
            return pipelineResult;
        }

        const SanitizationCapability capability = detectSanitizationCapability(device);
        const SanitizationMethod method = sanitizationEngine_.selectMethod(device, capability);
        initialResult.method = method;

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::METHOD_SELECTED,
                method == SanitizationMethod::Unsupported ? auditErrorSeverity() : SecureWipe::AuditSeverity::INFO,
                initialResult,
                requestId,
                actorId,
                method == SanitizationMethod::Unsupported ?
                    "No supported sanitization method was detected." :
                    "Sanitization method selected.",
                auditError))
        {
            auditOk = false;
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
            pipelineResult.sanitization.message = "Audit persistence failed after method selection: " + auditError;
            pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;
            pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
            pipelineResult.auditTrailPersisted = false;
            return pipelineResult;
        }

        if (method == SanitizationMethod::Unsupported)
        {
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::UNSUPPORTED_SANITIZATION_METHOD;
            pipelineResult.sanitization.message = "No supported sanitization method found for this target.";
            pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;

            if (!this->appendAudit(
                    SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                    auditErrorSeverity(),
                    pipelineResult.sanitization,
                    requestId,
                    actorId,
                    pipelineResult.sanitization.message,
                    auditError))
                auditOk = false;

            operationLogger_.error("Unsupported sanitization method for device=" + device.getDeviceId(), logError);
            pipelineResult.auditTrailPersisted = auditOk;
            pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
            return pipelineResult;
        }

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::SANITIZATION_STARTED,
                SecureWipe::AuditSeverity::INFO,
                initialResult,
                requestId,
                actorId,
                "Sanitization execution started.",
                auditError))
        {
            auditOk = false;
            pipelineResult.sanitization = initialResult;
            pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
            pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
            pipelineResult.sanitization.message = "Audit persistence failed immediately before sanitization: " + auditError;
            pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;
            pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
            pipelineResult.auditTrailPersisted = false;
            return pipelineResult;
        }

        operationLogger_.info("Sanitization execution started. device=" + device.getDeviceId(), logError);

        pipelineResult.sanitization = sanitizationEngine_.sanitize(device, safetyResult);

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::SANITIZATION_COMPLETED,
                pipelineResult.sanitization.status == SecureWipe::SanitizationStatus::COMPLETED ? SecureWipe::AuditSeverity::INFO : auditErrorSeverity(),
                pipelineResult.sanitization,
                requestId,
                actorId,
                pipelineResult.sanitization.message,
                auditError))
            auditOk = false;

        const SecureWipe::AuditSeverity verificationSeverity =
            pipelineResult.sanitization.verificationStatus == SecureWipe::VerificationStatus::PASSED ?
                SecureWipe::AuditSeverity::INFO :
                auditErrorSeverity();

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::VERIFICATION_COMPLETED,
                verificationSeverity,
                pipelineResult.sanitization,
                requestId,
                actorId,
                pipelineResult.sanitization.verificationMessage,
                auditError))
            auditOk = false;

        operationLogger_.info(
            "Sanitization execution finished. operationId=" + pipelineResult.sanitization.operationId,
            logError);

        if (!pipelineResult.sanitization.isSuccess())
        {
            if (pipelineResult.sanitization.errorMessage.empty())
                pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;

            const std::string failureMessage =
                "Sanitization did not reach a verified-success state. " +
                pipelineResult.sanitization.message;

            if (!this->appendAudit(
                    SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                    auditErrorSeverity(),
                    pipelineResult.sanitization,
                    requestId,
                    actorId,
                    failureMessage,
                    auditError))
                auditOk = false;

            operationLogger_.error(failureMessage, logError);
            pipelineResult.auditTrailPersisted = auditOk;
            pipelineResult.pipelineMessage = "Sanitization/verification failed. Certificate was not issued.";
            return pipelineResult;
        }

        pipelineResult.certificate =
            certificateGenerator_.generate(pipelineResult.sanitization, requestId);

        pipelineResult.certificateGenerated =
            pipelineResult.certificate.isValid();

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::CERTIFICATE_GENERATED,
                pipelineResult.certificateGenerated ? SecureWipe::AuditSeverity::INFO : auditErrorSeverity(),
                pipelineResult.sanitization,
                requestId,
                actorId,
                pipelineResult.certificate.message,
                auditError))
            auditOk = false;

        if (!pipelineResult.certificateGenerated)
        {
            if (!this->appendAudit(
                    SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                    auditErrorSeverity(),
                    pipelineResult.sanitization,
                    requestId,
                    actorId,
                    "Generated certificate failed validation.",
                    auditError))
                auditOk = false;

            operationLogger_.error(
                "Certificate validation failed for operationId=" + pipelineResult.sanitization.operationId,
                logError);

            pipelineResult.auditTrailPersisted = auditOk;
            pipelineResult.pipelineMessage = "Certificate validation failed.";
            return pipelineResult;
        }

        pipelineResult.certificatePath =
            (certificateDirectory_ / (pipelineResult.certificate.certificateId + ".json")).string();

        std::string certificateError;
        pipelineResult.certificatePersisted =
            persistCertificate(pipelineResult.certificate, pipelineResult.certificatePath, certificateError);

        if (!pipelineResult.certificatePersisted)
        {
            const std::string failureMessage =
                "Certificate persistence failed: " + certificateError;

            if (!this->appendAudit(
                    SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                    auditErrorSeverity(),
                    pipelineResult.sanitization,
                    requestId,
                    actorId,
                    failureMessage,
                    auditError))
                auditOk = false;

            operationLogger_.error(failureMessage, logError);
            pipelineResult.auditTrailPersisted = auditOk;
            pipelineResult.pipelineMessage = failureMessage;
            return pipelineResult;
        }

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::CERTIFICATE_PERSISTED,
                SecureWipe::AuditSeverity::INFO,
                pipelineResult.sanitization,
                requestId,
                actorId,
                "Certificate persisted to " + pipelineResult.certificatePath,
                auditError))
            auditOk = false;

        const bool completionAudit =
            this->appendAudit(
                SecureWipe::SanitizationAuditEvent::PIPELINE_COMPLETED,
                SecureWipe::AuditSeverity::INFO,
                pipelineResult.sanitization,
                requestId,
                actorId,
                "Host Overwrite vertical slice completed successfully.",
                auditError);

        if (!completionAudit)
            auditOk = false;

        pipelineResult.auditTrailPersisted = auditOk;

        operationLogger_.info(
            "Host Overwrite vertical pipeline completed. operationId=" +
            pipelineResult.sanitization.operationId +
            ", certificateId=" + pipelineResult.certificate.certificateId,
            logError);

        pipelineResult.pipelineMessage =
            pipelineResult.auditTrailPersisted ?
                "Sanitization, verification, certificate and audit persistence completed successfully." :
                "Sanitization succeeded, but the audit trail is incomplete.";

        return pipelineResult;
    }
    catch (const std::exception& exception)
    {
        pipelineResult.sanitization = initialResult;
        pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
        pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
        pipelineResult.sanitization.message = exception.what();
        pipelineResult.sanitization.errorMessage = exception.what();

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                auditErrorSeverity(),
                pipelineResult.sanitization,
                requestId,
                actorId,
                exception.what(),
                auditError))
            auditOk = false;

        pipelineResult.auditTrailPersisted = auditOk;
        pipelineResult.pipelineMessage = exception.what();
        return pipelineResult;
    }
    catch (...)
    {
        pipelineResult.sanitization = initialResult;
        pipelineResult.sanitization.status = SecureWipe::SanitizationStatus::FAILED;
        pipelineResult.sanitization.error = SecureWipe::SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED;
        pipelineResult.sanitization.message = "Unknown exception in sanitization pipeline.";
        pipelineResult.sanitization.errorMessage = pipelineResult.sanitization.message;

        if (!this->appendAudit(
                SecureWipe::SanitizationAuditEvent::PIPELINE_FAILED,
                auditErrorSeverity(),
                pipelineResult.sanitization,
                requestId,
                actorId,
                pipelineResult.sanitization.message,
                auditError))
            auditOk = false;

        pipelineResult.auditTrailPersisted = auditOk;
        pipelineResult.pipelineMessage = pipelineResult.sanitization.message;
        return pipelineResult;
    }
}

bool SanitizationPipeline::appendAudit(
    SecureWipe::SanitizationAuditEvent eventType,
    SecureWipe::AuditSeverity severity,
    const SecureWipe::SanitizationResult& result,
    const std::string& requestId,
    const std::string& actorId,
    const std::string& message,
    std::string& errorMessage,
    const SafetyResult* safetyResult)
{
    SecureWipe::SanitizationEvent event;
    event.eventType = eventType;
    event.severity = severity;
    event.operationId = result.operationId;
    event.requestId = requestId;
    event.actorId = actorId;
    event.deviceId = result.deviceId;
    event.model = result.model;
    event.serialNumber = result.serialNumber;
    event.interfaceType = result.interfaceType;
    event.capacityBytes = result.capacityBytes;
    event.method = result.method;
    event.sanitizationStatus = result.status;
    event.verificationStatus = result.verificationStatus;
    event.bytesProcessed = result.bytesProcessed;
    event.bytesVerified = result.bytesVerified;
    event.verificationSamples = result.verificationSamples;
    event.error = result.error;
    event.nativeErrorCode = result.nativeErrorCode;
    event.message = message;

    if (safetyResult != nullptr)
    {
        event.safetyDecision = safetyResult->decision;
        event.safetySummary = safetyResult->summary;
        event.safetyChecks = safetyResult->checks;
    }

    return auditLogger_.append(std::move(event), errorMessage);
}

bool SanitizationPipeline::persistCertificate(
    const SecureWipe::SanitizationCertificate& certificate,
    const std::filesystem::path& path,
    std::string& errorMessage) const
{
    try
    {
        const auto parent = path.parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent);

        std::ofstream output(path, std::ios::out | std::ios::trunc | std::ios::binary);

        if (!output)
        {
            errorMessage = "Unable to create certificate file: " + path.string();
            return false;
        }

        output << "{\n";
        output << "  \"certificateId\": \"" << escapeJson(certificate.certificateId) << "\",\n";
        output << "  \"operationId\": \"" << escapeJson(certificate.operationId) << "\",\n";
        output << "  \"requestId\": \"" << escapeJson(certificate.requestId) << "\",\n";
        output << "  \"deviceId\": \"" << escapeJson(certificate.deviceId) << "\",\n";
        output << "  \"model\": \"" << escapeJson(certificate.model) << "\",\n";
        output << "  \"serialNumber\": \"" << escapeJson(certificate.serialNumber) << "\",\n";
        output << "  \"capacityBytes\": " << certificate.capacityBytes << ",\n";
        output << "  \"interfaceType\": \"" << escapeJson(certificate.interfaceType) << "\",\n";
        output << "  \"method\": \"" << SecureWipe::SanitizationEvent::methodName(certificate.method) << "\",\n";
        output << "  \"status\": \"" << SecureWipe::SanitizationEvent::sanitizationStatusName(certificate.status) << "\",\n";
        output << "  \"bytesProcessed\": " << certificate.bytesProcessed << ",\n";
        output << "  \"operationDurationMs\": " << certificate.operationDurationMs << ",\n";
        output << "  \"verificationStatus\": \"" << SecureWipe::SanitizationEvent::verificationStatusName(certificate.verificationStatus) << "\",\n";
        output << "  \"verificationPerformed\": " << (certificate.verificationPerformed ? "true" : "false") << ",\n";
        output << "  \"verificationPassed\": " << (certificate.verificationPassed ? "true" : "false") << ",\n";
        output << "  \"bytesVerified\": " << certificate.bytesVerified << ",\n";
        output << "  \"verificationSamples\": " << certificate.verificationSamples << ",\n";
        output << "  \"deviceReportedSuccess\": " << (certificate.deviceReportedSuccess ? "true" : "false") << ",\n";
        output << "  \"globalDataErased\": " << (certificate.globalDataErased ? "true" : "false") << ",\n";
        output << "  \"nativeErrorCode\": " << certificate.nativeErrorCode << ",\n";
        output << "  \"verificationMessage\": \"" << escapeJson(certificate.verificationMessage) << "\",\n";
        output << "  \"generatedAt\": \"" << escapeJson(certificate.generatedAt) << "\",\n";
        output << "  \"hashAlgorithm\": \"" << escapeJson(certificate.hashAlgorithm) << "\",\n";
        output << "  \"certificateHash\": \"" << escapeJson(certificate.certificateHash) << "\",\n";
        output << "  \"message\": \"" << escapeJson(certificate.message) << "\"\n";
        output << "}\n";
        output.flush();

        if (!output.good())
        {
            errorMessage = "Failed while writing certificate file: " + path.string();
            return false;
        }

        return true;
    }
    catch (const std::exception& exception)
    {
        errorMessage = exception.what();
        return false;
    }
}
