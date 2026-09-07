#include "SanitizationEvent.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace
{
std::string escapeJson(const std::string& value)
{
    std::ostringstream output;

    for (const char character : value)
    {
        switch (character)
        {
        case '\\':
            output << "\\\\";
            break;

        case '"':
            output << "\\\"";
            break;

        case '\n':
            output << "\\n";
            break;

        case '\r':
            output << "\\r";
            break;

        case '\t':
            output << "\\t";
            break;

        default:
            output << character;
            break;
        }
    }

    return output.str();
}
}

namespace SecureWipe
{
const char* SanitizationEvent::eventTypeName(
    SanitizationAuditEvent eventType)
{
    switch (eventType)
    {
    case SanitizationAuditEvent::PIPELINE_STARTED:
        return "PIPELINE_STARTED";

    case SanitizationAuditEvent::TARGET_VALIDATED:
        return "TARGET_VALIDATED";

    case SanitizationAuditEvent::SAFETY_CHECK_COMPLETED:
        return "SAFETY_CHECK_COMPLETED";

    case SanitizationAuditEvent::METHOD_SELECTED:
        return "METHOD_SELECTED";

    case SanitizationAuditEvent::SANITIZATION_STARTED:
        return "SANITIZATION_STARTED";

    case SanitizationAuditEvent::SANITIZATION_COMPLETED:
        return "SANITIZATION_COMPLETED";

    case SanitizationAuditEvent::VERIFICATION_COMPLETED:
        return "VERIFICATION_COMPLETED";

    case SanitizationAuditEvent::CERTIFICATE_GENERATED:
        return "CERTIFICATE_GENERATED";

    case SanitizationAuditEvent::CERTIFICATE_PERSISTED:
        return "CERTIFICATE_PERSISTED";

    case SanitizationAuditEvent::PIPELINE_COMPLETED:
        return "PIPELINE_COMPLETED";

    case SanitizationAuditEvent::PIPELINE_FAILED:
        return "PIPELINE_FAILED";
    }

    return "UNKNOWN";
}

const char* SanitizationEvent::severityName(
    AuditSeverity severity)
{
    switch (severity)
    {
    case AuditSeverity::INFO:
        return "INFO";

    case AuditSeverity::WARNING:
        return "WARNING";

    case AuditSeverity::FAILURE:
        return "FAILURE";
    }

    return "UNKNOWN";
}

const char* SanitizationEvent::methodName(
    SanitizationMethod method)
{
    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
        return "NVME_SANITIZE";

    case SanitizationMethod::AtaSanitize:
        return "ATA_SANITIZE";

    case SanitizationMethod::HostOverwrite:
        return "HOST_OVERWRITE";

    case SanitizationMethod::Unsupported:
    default:
        return "UNSUPPORTED";
    }
}

const char* SanitizationEvent::sanitizationStatusName(
    SanitizationStatus status)
{
    switch (status)
    {
    case SanitizationStatus::NOT_STARTED:
        return "NOT_STARTED";

    case SanitizationStatus::IN_PROGRESS:
        return "IN_PROGRESS";

    case SanitizationStatus::COMPLETED:
        return "COMPLETED";

    case SanitizationStatus::FAILED:
        return "FAILED";

    case SanitizationStatus::ABORTED:
        return "ABORTED";
    }

    return "UNKNOWN";
}

const char* SanitizationEvent::verificationStatusName(
    VerificationStatus status)
{
    switch (status)
    {
    case VerificationStatus::NOT_PERFORMED:
        return "NOT_PERFORMED";

    case VerificationStatus::IN_PROGRESS:
        return "IN_PROGRESS";

    case VerificationStatus::PASSED:
        return "PASSED";

    case VerificationStatus::FAILED:
        return "FAILED";
    }

    return "UNKNOWN";
}

const char* SanitizationEvent::errorCodeName(
    SanitizationErrorCode error)
{
    switch (error)
    {
    case SanitizationErrorCode::NONE:
        return "NONE";

    case SanitizationErrorCode::SAFETY_VALIDATION_FAILED:
        return "SAFETY_VALIDATION_FAILED";

    case SanitizationErrorCode::MISSING_DEVICE_ID:
        return "MISSING_DEVICE_ID";

    case SanitizationErrorCode::UNKNOWN_DEVICE_CAPACITY:
        return "UNKNOWN_DEVICE_CAPACITY";

    case SanitizationErrorCode::DEVICE_OPEN_FAILED:
        return "DEVICE_OPEN_FAILED";

    case SanitizationErrorCode::UNSUPPORTED_SANITIZATION_METHOD:
        return "UNSUPPORTED_SANITIZATION_METHOD";

    case SanitizationErrorCode::NVME_ALGORITHM_UNAVAILABLE:
        return "NVME_ALGORITHM_UNAVAILABLE";

    case SanitizationErrorCode::ATA_ALGORITHM_UNAVAILABLE:
        return "ATA_ALGORITHM_UNAVAILABLE";

    case SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED:
        return "SANITIZATION_EXECUTION_FAILED";

    case SanitizationErrorCode::VERIFICATION_FAILED:
        return "VERIFICATION_FAILED";
    }

    return "UNKNOWN";
}

std::string SanitizationEvent::generateEventId()
{
    const auto now =
        std::chrono::system_clock::now();

    const auto micros =
        std::chrono::duration_cast<
            std::chrono::microseconds>(
            now.time_since_epoch()).count();

    return "SW-AUDIT-" +
           std::to_string(
               static_cast<unsigned long long>(
                   micros));
}

std::string SanitizationEvent::generateTimestampUtc()
{
    const auto now =
        std::chrono::system_clock::now();

    const std::time_t timeValue =
        std::chrono::system_clock::to_time_t(now);

    std::tm utcTime{};

#ifdef _WIN32
    gmtime_s(&utcTime, &timeValue);
#else
    gmtime_r(&timeValue, &utcTime);
#endif

    std::ostringstream output;

    output << std::put_time(
        &utcTime,
        "%Y-%m-%dT%H:%M:%SZ");

    return output.str();
}

std::string SanitizationEvent::canonicalData() const
{
    std::ostringstream output;

    output << "eventId=" << eventId << '\n';
    output << "timestampUtc=" << timestampUtc << '\n';
    output << "eventType=" << eventTypeName(eventType) << '\n';
    output << "severity=" << severityName(severity) << '\n';
    output << "operationId=" << operationId << '\n';
    output << "requestId=" << requestId << '\n';
    output << "actorId=" << actorId << '\n';
    output << "deviceId=" << deviceId << '\n';
    output << "model=" << model << '\n';
    output << "serialNumber=" << serialNumber << '\n';
    output << "interfaceType=" << interfaceType << '\n';
    output << "capacityBytes=" << capacityBytes << '\n';
    output << "method=" << methodName(method) << '\n';
    output << "sanitizationStatus="
           << sanitizationStatusName(
                  sanitizationStatus)
           << '\n';
    output << "verificationStatus="
           << verificationStatusName(
                  verificationStatus)
           << '\n';
    output << "bytesProcessed=" << bytesProcessed << '\n';
    output << "bytesVerified=" << bytesVerified << '\n';
    output << "verificationSamples="
           << verificationSamples << '\n';
    output << "error=" << errorCodeName(error) << '\n';
    output << "nativeErrorCode="
           << nativeErrorCode << '\n';
    output << "safetyDecision="
           << safetyDecision << '\n';
    output << "safetySummary="
           << safetySummary << '\n';

    for (const auto& check : safetyChecks)
    {
        output << "safetyCheck="
               << check.checkName
               << '|'
               << (check.passed
                       ? "true"
                       : "false")
               << '|'
               << check.message
               << '\n';
    }

    output << "message=" << message << '\n';

    output << "previousEventHash="
           << previousEventHash
           << '\n';

    return output.str();
}

std::string SanitizationEvent::toJsonLine() const
{
    std::ostringstream output;

    output << '{';

    output << "\"eventId\":\""
           << escapeJson(eventId)
           << "\",";

    output << "\"timestampUtc\":\""
           << escapeJson(timestampUtc)
           << "\",";

    output << "\"eventType\":\""
           << eventTypeName(eventType)
           << "\",";

    output << "\"severity\":\""
           << severityName(severity)
           << "\",";

    output << "\"operationId\":\""
           << escapeJson(operationId)
           << "\",";

    output << "\"requestId\":\""
           << escapeJson(requestId)
           << "\",";

    output << "\"actorId\":\""
           << escapeJson(actorId)
           << "\",";

    output << "\"deviceId\":\""
           << escapeJson(deviceId)
           << "\",";

    output << "\"model\":\""
           << escapeJson(model)
           << "\",";

    output << "\"serialNumber\":\""
           << escapeJson(serialNumber)
           << "\",";

    output << "\"interfaceType\":\""
           << escapeJson(interfaceType)
           << "\",";

    output << "\"capacityBytes\":"
           << capacityBytes
           << ',';

    output << "\"method\":\""
           << methodName(method)
           << "\",";

    output << "\"sanitizationStatus\":\""
           << sanitizationStatusName(
                  sanitizationStatus)
           << "\",";

    output << "\"verificationStatus\":\""
           << verificationStatusName(
                  verificationStatus)
           << "\",";

    output << "\"bytesProcessed\":"
           << bytesProcessed
           << ',';

    output << "\"bytesVerified\":"
           << bytesVerified
           << ',';

    output << "\"verificationSamples\":"
           << verificationSamples
           << ',';

    output << "\"error\":\""
           << errorCodeName(error)
           << "\",";

    output << "\"nativeErrorCode\":"
           << nativeErrorCode
           << ',';

    output << "\"safetyDecision\":\""
           << escapeJson(safetyDecision)
           << "\",";

    output << "\"safetySummary\":\""
           << escapeJson(safetySummary)
           << "\",";

    output << "\"safetyChecks\":[";

    for (std::size_t index = 0;
         index < safetyChecks.size();
         ++index)
    {
        if (index > 0)
            output << ',';

        output << "{\"name\":\""
               << escapeJson(
                      safetyChecks[index].checkName)
               << "\",\"passed\":"
               << (safetyChecks[index].passed
                       ? "true"
                       : "false")
               << ",\"message\":\""
               << escapeJson(
                      safetyChecks[index].message)
               << "\"}";
    }

    output << "],";

    output << "\"message\":\""
           << escapeJson(message)
           << "\",";

    output << "\"previousEventHash\":\""
           << escapeJson(previousEventHash)
           << "\",";

    output << "\"eventHash\":\""
           << escapeJson(eventHash)
           << "\"";

    output << '}';

    return output.str();
}
}