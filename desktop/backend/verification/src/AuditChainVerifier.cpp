#include "AuditChainVerifier.h"

#include <Windows.h>
#include <bcrypt.h>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace
{
    struct JsonValue
    {
        enum class Type
        {
            Null,
            String,
            Number,
            Boolean,
            Object,
            Array
        };

        Type type = Type::Null;
        std::string stringValue;
        std::map<std::string, JsonValue> objectValue;
        std::vector<JsonValue> arrayValue;
    };

    class JsonParser
    {
    public:
        explicit JsonParser(const std::string& input)
            : input_(input), position_(0)
        {
        }

        bool parse(JsonValue& value, std::string& error)
        {
            skipWhitespace();

            if (!parseValue(value, error))
                return false;

            skipWhitespace();

            if (position_ != input_.size())
            {
                error = "Unexpected trailing JSON data.";
                return false;
            }

            return true;
        }

    private:
        const std::string& input_;
        std::size_t position_;

        void skipWhitespace()
        {
            while (position_ < input_.size())
            {
                const char c = input_[position_];

                if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                    ++position_;
                else
                    break;
            }
        }

        bool consume(char expected)
        {
            skipWhitespace();

            if (position_ >= input_.size() ||
                input_[position_] != expected)
            {
                return false;
            }

            ++position_;
            return true;
        }

        bool parseValue(JsonValue& value, std::string& error)
        {
            skipWhitespace();

            if (position_ >= input_.size())
            {
                error = "Unexpected end of JSON.";
                return false;
            }

            const char c = input_[position_];

            if (c == '"')
            {
                value.type = JsonValue::Type::String;
                return parseString(value.stringValue, error);
            }

            if (c == '{')
                return parseObject(value, error);

            if (c == '[')
                return parseArray(value, error);

            if (c == 't')
            {
                if (matchLiteral("true"))
                {
                    value.type = JsonValue::Type::Boolean;
                    value.stringValue = "true";
                    return true;
                }

                error = "Invalid boolean value.";
                return false;
            }

            if (c == 'f')
            {
                if (matchLiteral("false"))
                {
                    value.type = JsonValue::Type::Boolean;
                    value.stringValue = "false";
                    return true;
                }

                error = "Invalid boolean value.";
                return false;
            }

            if (c == 'n')
            {
                if (matchLiteral("null"))
                {
                    value.type = JsonValue::Type::Null;
                    return true;
                }

                error = "Invalid null value.";
                return false;
            }

            if (c == '-' || (c >= '0' && c <= '9'))
            {
                value.type = JsonValue::Type::Number;
                return parseNumber(value.stringValue, error);
            }

            error = "Unexpected JSON token.";
            return false;
        }

        bool matchLiteral(const char* literal)
        {
            const std::size_t length = std::strlen(literal);

            if (position_ + length > input_.size())
                return false;

            if (input_.compare(position_, length, literal) != 0)
                return false;

            position_ += length;
            return true;
        }

        bool parseString(std::string& output, std::string& error)
        {
            if (!consume('"'))
            {
                error = "Expected JSON string.";
                return false;
            }

            output.clear();

            while (position_ < input_.size())
            {
                const char c = input_[position_++];

                if (c == '"')
                    return true;

                if (c == '\\')
                {
                    if (position_ >= input_.size())
                    {
                        error = "Invalid JSON escape sequence.";
                        return false;
                    }

                    const char escape = input_[position_++];

                    switch (escape)
                    {
                    case '"': output.push_back('"'); break;
                    case '\\': output.push_back('\\'); break;
                    case '/': output.push_back('/'); break;
                    case 'b': output.push_back('\b'); break;
                    case 'f': output.push_back('\f'); break;
                    case 'n': output.push_back('\n'); break;
                    case 'r': output.push_back('\r'); break;
                    case 't': output.push_back('\t'); break;
                    case 'u':
                        error = "Unicode JSON escape is not supported by this verifier.";
                        return false;
                    default:
                        error = "Unknown JSON escape sequence.";
                        return false;
                    }
                }
                else
                {
                    output.push_back(c);
                }
            }

            error = "Unterminated JSON string.";
            return false;
        }

        bool parseNumber(std::string& output, std::string& error)
        {
            const std::size_t start = position_;

            if (input_[position_] == '-')
                ++position_;

            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }

            if (position_ < input_.size() && input_[position_] == '.')
            {
                ++position_;

                while (position_ < input_.size() &&
                       input_[position_] >= '0' &&
                       input_[position_] <= '9')
                {
                    ++position_;
                }
            }

            if (position_ < input_.size() &&
                (input_[position_] == 'e' ||
                 input_[position_] == 'E'))
            {
                ++position_;

                if (position_ < input_.size() &&
                    (input_[position_] == '+' ||
                     input_[position_] == '-'))
                {
                    ++position_;
                }

                while (position_ < input_.size() &&
                       input_[position_] >= '0' &&
                       input_[position_] <= '9')
                {
                    ++position_;
                }
            }

            if (start == position_)
            {
                error = "Invalid JSON number.";
                return false;
            }

            output = input_.substr(
                start,
                position_ - start);

            return true;
        }

        bool parseObject(JsonValue& value, std::string& error)
        {
            if (!consume('{'))
            {
                error = "Expected JSON object.";
                return false;
            }

            value.type = JsonValue::Type::Object;
            value.objectValue.clear();

            skipWhitespace();

            if (position_ < input_.size() &&
                input_[position_] == '}')
            {
                ++position_;
                return true;
            }

            while (position_ < input_.size())
            {
                skipWhitespace();

                std::string key;

                if (!parseString(key, error))
                    return false;

                skipWhitespace();

                if (!consume(':'))
                {
                    error = "Expected ':' after JSON object key.";
                    return false;
                }

                JsonValue child;

                if (!parseValue(child, error))
                    return false;

                value.objectValue[key] = std::move(child);

                skipWhitespace();

                if (position_ < input_.size() &&
                    input_[position_] == '}')
                {
                    ++position_;
                    return true;
                }

                if (!consume(','))
                {
                    error = "Expected ',' between JSON object members.";
                    return false;
                }
            }

            error = "Unterminated JSON object.";
            return false;
        }

        bool parseArray(JsonValue& value, std::string& error)
        {
            if (!consume('['))
            {
                error = "Expected JSON array.";
                return false;
            }

            value.type = JsonValue::Type::Array;
            value.arrayValue.clear();

            skipWhitespace();

            if (position_ < input_.size() &&
                input_[position_] == ']')
            {
                ++position_;
                return true;
            }

            while (position_ < input_.size())
            {
                JsonValue child;

                if (!parseValue(child, error))
                    return false;

                value.arrayValue.push_back(std::move(child));

                skipWhitespace();

                if (position_ < input_.size() &&
                    input_[position_] == ']')
                {
                    ++position_;
                    return true;
                }

                if (!consume(','))
                {
                    error = "Expected ',' between JSON array elements.";
                    return false;
                }
            }

            error = "Unterminated JSON array.";
            return false;
        }
    };

    bool getString(
        const JsonValue& object,
        const std::string& key,
        std::string& value,
        std::string& error)
    {
        const auto iterator = object.objectValue.find(key);

        if (iterator == object.objectValue.end())
        {
            error = "Missing JSON field: " + key;
            return false;
        }

        if (iterator->second.type != JsonValue::Type::String)
        {
            error = "JSON field is not a string: " + key;
            return false;
        }

        value = iterator->second.stringValue;
        return true;
    }

    bool getScalar(
        const JsonValue& object,
        const std::string& key,
        std::string& value,
        std::string& error)
    {
        const auto iterator = object.objectValue.find(key);

        if (iterator == object.objectValue.end())
        {
            error = "Missing JSON field: " + key;
            return false;
        }

        const JsonValue& child = iterator->second;

        if (child.type == JsonValue::Type::String ||
            child.type == JsonValue::Type::Number ||
            child.type == JsonValue::Type::Boolean)
        {
            value = child.stringValue;
            return true;
        }

        error = "JSON field is not scalar: " + key;
        return false;
    }

    bool getArray(
        const JsonValue& object,
        const std::string& key,
        const std::vector<JsonValue>*& value,
        std::string& error)
    {
        const auto iterator = object.objectValue.find(key);

        if (iterator == object.objectValue.end())
        {
            error = "Missing JSON field: " + key;
            return false;
        }

        if (iterator->second.type != JsonValue::Type::Array)
        {
            error = "JSON field is not an array: " + key;
            return false;
        }

        value = &iterator->second.arrayValue;
        return true;
    }

    std::string buildCanonicalAuditData(
        const JsonValue& object,
        std::string& error)
    {
        std::string eventId;
        std::string timestampUtc;
        std::string eventType;
        std::string severity;
        std::string operationId;
        std::string requestId;
        std::string actorId;
        std::string workstationId;
        bool hasWorkstationId = false;
        std::string deviceId;
        std::string model;
        std::string serialNumber;
        std::string interfaceType;
        std::string capacityBytes;
        std::string method;
        std::string sanitizationStatus;
        std::string verificationStatus;
        std::string bytesProcessed;
        std::string bytesVerified;
        std::string verificationSamples;
        std::string eventError;
        std::string nativeErrorCode;
        std::string safetyDecision;
        std::string safetySummary;
        std::string message;
        std::string previousEventHash;

        if (!getString(object, "eventId", eventId, error) ||
            !getString(object, "timestampUtc", timestampUtc, error) ||
            !getString(object, "eventType", eventType, error) ||
            !getString(object, "severity", severity, error) ||
            !getString(object, "operationId", operationId, error) ||
            !getString(object, "requestId", requestId, error) ||
            !getString(object, "actorId", actorId, error) ||
            !getString(object, "deviceId", deviceId, error) ||
            !getString(object, "model", model, error) ||
            !getString(object, "serialNumber", serialNumber, error) ||
            !getString(object, "interfaceType", interfaceType, error) ||
            !getScalar(object, "capacityBytes", capacityBytes, error) ||
            !getString(object, "method", method, error) ||
            !getString(object, "sanitizationStatus", sanitizationStatus, error) ||
            !getString(object, "verificationStatus", verificationStatus, error) ||
            !getScalar(object, "bytesProcessed", bytesProcessed, error) ||
            !getScalar(object, "bytesVerified", bytesVerified, error) ||
            !getScalar(object, "verificationSamples", verificationSamples, error) ||
            !getString(object, "error", eventError, error) ||
            !getScalar(object, "nativeErrorCode", nativeErrorCode, error) ||
            !getString(object, "safetyDecision", safetyDecision, error) ||
            !getString(object, "safetySummary", safetySummary, error) ||
            !getString(object, "message", message, error) ||
            !getString(object, "previousEventHash", previousEventHash, error))
        {
            return {};
        }

        const auto workstationIterator =
            object.objectValue.find("workstationId");

        if (workstationIterator != object.objectValue.end())
        {
            if (workstationIterator->second.type !=
                JsonValue::Type::String)
            {
                error = "JSON field is not a string: workstationId";
                return {};
            }

            workstationId =
                workstationIterator->second.stringValue;
            hasWorkstationId = true;
        }

        const std::vector<JsonValue>* safetyChecks = nullptr;

        if (!getArray(
                object,
                "safetyChecks",
                safetyChecks,
                error))
        {
            return {};
        }

        std::ostringstream canonical;

        canonical << "eventId=" << eventId << '\n';
        canonical << "timestampUtc=" << timestampUtc << '\n';
        canonical << "eventType=" << eventType << '\n';
        canonical << "severity=" << severity << '\n';
        canonical << "operationId=" << operationId << '\n';
        canonical << "requestId=" << requestId << '\n';
        canonical << "actorId=" << actorId << '\n';

        if (hasWorkstationId)
            canonical << "workstationId=" << workstationId << '\n';

        canonical << "deviceId=" << deviceId << '\n';
        canonical << "model=" << model << '\n';
        canonical << "serialNumber=" << serialNumber << '\n';
        canonical << "interfaceType=" << interfaceType << '\n';
        canonical << "capacityBytes=" << capacityBytes << '\n';
        canonical << "method=" << method << '\n';
        canonical << "sanitizationStatus=" << sanitizationStatus << '\n';
        canonical << "verificationStatus=" << verificationStatus << '\n';
        canonical << "bytesProcessed=" << bytesProcessed << '\n';
        canonical << "bytesVerified=" << bytesVerified << '\n';
        canonical << "verificationSamples=" << verificationSamples << '\n';
        canonical << "error=" << eventError << '\n';
        canonical << "nativeErrorCode=" << nativeErrorCode << '\n';
        canonical << "safetyDecision=" << safetyDecision << '\n';
        canonical << "safetySummary=" << safetySummary << '\n';

        for (const JsonValue& check : *safetyChecks)
        {
            if (check.type != JsonValue::Type::Object)
            {
                error = "A safety check is not a JSON object.";
                return {};
            }

            std::string checkName;
            std::string passed;

            if (!getString(check, "name", checkName, error) ||
                !getScalar(check, "passed", passed, error))
            {
                return {};
            }

            canonical << "safetyCheck="
                      << checkName
                      << '|'
                      << passed
                      << '|';

            std::string checkMessage;

            if (!getString(
                    check,
                    "message",
                    checkMessage,
                    error))
            {
                return {};
            }

            canonical << checkMessage << '\n';
        }

        canonical << "message=" << message << '\n';
        canonical << "previousEventHash=" << previousEventHash << '\n';

        return canonical.str();
    }

    bool readEventHash(
        const JsonValue& object,
        std::string& value,
        std::string& error)
    {
        return getString(
            object,
            "eventHash",
            value,
            error);
    }

    bool readPreviousHash(
        const JsonValue& object,
        std::string& value,
        std::string& error)
    {
        return getString(
            object,
            "previousEventHash",
            value,
            error);
    }
}

std::string AuditChainVerifier::calculateSha256(
    const std::string& data)
{
    BCRYPT_ALG_HANDLE algorithmHandle = nullptr;
    BCRYPT_HASH_HANDLE hashHandle = nullptr;

    DWORD objectSize = 0;
    DWORD hashSize = 0;
    DWORD resultSize = 0;

    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithmHandle,
        BCRYPT_SHA256_ALGORITHM,
        nullptr,
        0);

    if (status < 0)
        throw std::runtime_error(
            "Failed to open BCrypt SHA-256 provider.");

    status = BCryptGetProperty(
        algorithmHandle,
        BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectSize),
        sizeof(objectSize),
        &resultSize,
        0);

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        throw std::runtime_error(
            "Failed to query BCrypt hash object size.");
    }

    status = BCryptGetProperty(
        algorithmHandle,
        BCRYPT_HASH_LENGTH,
        reinterpret_cast<PUCHAR>(&hashSize),
        sizeof(hashSize),
        &resultSize,
        0);

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        throw std::runtime_error(
            "Failed to query BCrypt SHA-256 hash length.");
    }

    std::vector<UCHAR> hashObject(objectSize);
    std::vector<UCHAR> hash(hashSize);

    status = BCryptCreateHash(
        algorithmHandle,
        &hashHandle,
        hashObject.data(),
        objectSize,
        nullptr,
        0,
        0);

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        throw std::runtime_error(
            "Failed to create BCrypt SHA-256 hash.");
    }

    status = BCryptHashData(
        hashHandle,
        reinterpret_cast<PUCHAR>(
            const_cast<char*>(data.data())),
        static_cast<ULONG>(data.size()),
        0);

    if (status < 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        throw std::runtime_error(
            "Failed to hash audit event.");
    }

    status = BCryptFinishHash(
        hashHandle,
        hash.data(),
        hashSize,
        0);

    if (status < 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        throw std::runtime_error(
            "Failed to finalize audit hash.");
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0');

    for (const UCHAR byte : hash)
    {
        stream << std::setw(2)
               << static_cast<unsigned int>(byte);
    }

    BCryptDestroyHash(hashHandle);
    BCryptCloseAlgorithmProvider(
        algorithmHandle,
        0);

    return stream.str();
}

AuditChainVerificationResult AuditChainVerifier::verify(
    const std::filesystem::path& auditLogPath) const
{
    AuditChainVerificationResult result;

    if (auditLogPath.empty())
    {
        result.message = "Audit log path is empty.";
        return result;
    }

    if (!std::filesystem::exists(auditLogPath))
    {
        result.message =
            "Audit log file does not exist: " +
            auditLogPath.string();

        return result;
    }

    if (!verifyEventChain(
            auditLogPath,
            result))
    {
        return result;
    }

    result.valid = true;
    result.message =
        "Audit chain verification passed for all events.";

    return result;
}

bool AuditChainVerifier::verifyEventChain(
    const std::filesystem::path& auditLogPath,
    AuditChainVerificationResult& result)
{
    std::ifstream input(
        auditLogPath,
        std::ios::in |
        std::ios::binary);

    if (!input)
    {
        result.message =
            "Unable to open audit log: " +
            auditLogPath.string();

        return false;
    }

    std::string line;
    std::string previousEventHash;
    bool firstEvent = true;
    std::size_t lineNumber = 0;

    while (std::getline(input, line))
    {
        ++lineNumber;

        if (line.empty())
            continue;

        JsonValue root;
        std::string parseError;

        JsonParser parser(line);

        if (!parser.parse(root, parseError))
        {
            result.failedEventIndex = lineNumber;
            result.message =
                "Invalid JSON at audit event line " +
                std::to_string(lineNumber) +
                ": " +
                parseError;

            return false;
        }

        if (root.type != JsonValue::Type::Object)
        {
            result.failedEventIndex = lineNumber;
            result.message =
                "Audit event line is not a JSON object.";

            return false;
        }

        ++result.eventCount;

        std::string storedHash;
        std::string eventPreviousHash;

        if (!readEventHash(
                root,
                storedHash,
                parseError))
        {
            result.failedEventIndex = lineNumber;
            result.message = parseError;
            return false;
        }

        if (!readPreviousHash(
                root,
                eventPreviousHash,
                parseError))
        {
            result.failedEventIndex = lineNumber;
            result.message = parseError;
            return false;
        }

        if (firstEvent)
        {
            result.firstEventPreviousHash =
                eventPreviousHash;

            /*
             * The first event may reference a previous log segment
             * created before this verifier started. There is no
             * trusted genesis hash in the current format, so the
             * first event's previousEventHash cannot be independently
             * anchored here.
             */
            firstEvent = false;
        }
        else
        {
            if (eventPreviousHash != previousEventHash)
            {
                result.failedEventIndex = lineNumber;
                result.expectedHash = previousEventHash;
                result.actualHash = eventPreviousHash;
                result.message =
                    "Audit chain linkage failed at event line " +
                    std::to_string(lineNumber) +
                    ": previousEventHash does not match the preceding eventHash.";

                return false;
            }
        }

        std::string canonicalError;

        const std::string canonicalData =
            buildCanonicalAuditData(
                root,
                canonicalError);

        if (!canonicalError.empty())
        {
            result.failedEventIndex = lineNumber;
            result.message =
                "Unable to reconstruct canonical audit data at line " +
                std::to_string(lineNumber) +
                ": " +
                canonicalError;

            return false;
        }

        std::string calculatedHash;

        try
        {
            calculatedHash =
                AuditChainVerifier::calculateSha256(
                    canonicalData);
        }
        catch (const std::exception& exception)
        {
            result.failedEventIndex = lineNumber;
            result.message =
                "SHA-256 verification failed at line " +
                std::to_string(lineNumber) +
                ": " +
                exception.what();

            return false;
        }

        if (calculatedHash != storedHash)
        {
            result.failedEventIndex = lineNumber;
            result.expectedHash = calculatedHash;
            result.actualHash = storedHash;
            result.message =
                "Audit event hash mismatch at line " +
                std::to_string(lineNumber) +
                ".";

            return false;
        }

        previousEventHash = storedHash;
        ++result.verifiedEventCount;
    }

    if (result.eventCount == 0)
    {
        result.message =
            "Audit log contains no events.";

        return false;
    }

    return true;
}