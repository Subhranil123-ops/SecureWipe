#include "CertificateVerifier.h"

#include <Windows.h>
#include <bcrypt.h>

#include <cstdint>
#include <cstring>
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

                if (c == ' ' || c == '\t' ||
                    c == '\r' || c == '\n')
                {
                    ++position_;
                }
                else
                {
                    break;
                }
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

        bool parseValue(
            JsonValue& value,
            std::string& error)
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
                value.type =
                    JsonValue::Type::String;

                return parseString(
                    value.stringValue,
                    error);
            }

            if (c == '{')
                return parseObject(value, error);

            if (c == '[')
                return parseArray(value, error);

            if (c == 't')
            {
                if (matchLiteral("true"))
                {
                    value.type =
                        JsonValue::Type::Boolean;

                    value.stringValue =
                        "true";

                    return true;
                }

                error = "Invalid boolean.";
                return false;
            }

            if (c == 'f')
            {
                if (matchLiteral("false"))
                {
                    value.type =
                        JsonValue::Type::Boolean;

                    value.stringValue =
                        "false";

                    return true;
                }

                error = "Invalid boolean.";
                return false;
            }

            if (c == 'n')
            {
                if (matchLiteral("null"))
                {
                    value.type =
                        JsonValue::Type::Null;

                    return true;
                }

                error = "Invalid null.";
                return false;
            }

            if (c == '-' ||
                (c >= '0' && c <= '9'))
            {
                value.type =
                    JsonValue::Type::Number;

                return parseNumber(
                    value.stringValue,
                    error);
            }

            error = "Unexpected JSON token.";
            return false;
        }

        bool matchLiteral(const char* literal)
        {
            const std::size_t length =
                std::strlen(literal);

            if (position_ + length >
                input_.size())
            {
                return false;
            }

            if (input_.compare(
                    position_,
                    length,
                    literal) != 0)
            {
                return false;
            }

            position_ += length;
            return true;
        }

        bool parseString(
            std::string& output,
            std::string& error)
        {
            if (!consume('"'))
            {
                error =
                    "Expected JSON string.";

                return false;
            }

            output.clear();

            while (position_ < input_.size())
            {
                const char c =
                    input_[position_++];

                if (c == '"')
                    return true;

                if (c == '\\')
                {
                    if (position_ >= input_.size())
                    {
                        error =
                            "Invalid JSON escape.";

                        return false;
                    }

                    const char escape =
                        input_[position_++];

                    switch (escape)
                    {
                    case '"':
                        output.push_back('"');
                        break;

                    case '\\':
                        output.push_back('\\');
                        break;

                    case '/':
                        output.push_back('/');
                        break;

                    case 'b':
                        output.push_back('\b');
                        break;

                    case 'f':
                        output.push_back('\f');
                        break;

                    case 'n':
                        output.push_back('\n');
                        break;

                    case 'r':
                        output.push_back('\r');
                        break;

                    case 't':
                        output.push_back('\t');
                        break;

                    case 'u':
                        error =
                            "Unicode JSON escape is not supported by this verifier.";

                        return false;

                    default:
                        error =
                            "Unknown JSON escape.";

                        return false;
                    }
                }
                else
                {
                    output.push_back(c);
                }
            }

            error =
                "Unterminated JSON string.";

            return false;
        }

        bool parseNumber(
            std::string& output,
            std::string& error)
        {
            const std::size_t start =
                position_;

            if (input_[position_] == '-')
                ++position_;

            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }

            if (position_ < input_.size() &&
                input_[position_] == '.')
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
                error =
                    "Invalid JSON number.";

                return false;
            }

            output =
                input_.substr(
                    start,
                    position_ - start);

            return true;
        }

        bool parseObject(
            JsonValue& value,
            std::string& error)
        {
            if (!consume('{'))
            {
                error =
                    "Expected JSON object.";

                return false;
            }

            value.type =
                JsonValue::Type::Object;

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

                if (!consume(':'))
                {
                    error =
                        "Expected ':' after object key.";

                    return false;
                }

                JsonValue child;

                if (!parseValue(child, error))
                    return false;

                value.objectValue[key] =
                    std::move(child);

                skipWhitespace();

                if (position_ < input_.size() &&
                    input_[position_] == '}')
                {
                    ++position_;
                    return true;
                }

                if (!consume(','))
                {
                    error =
                        "Expected ',' in JSON object.";

                    return false;
                }
            }

            error =
                "Unterminated JSON object.";

            return false;
        }

        bool parseArray(
            JsonValue& value,
            std::string& error)
        {
            if (!consume('['))
            {
                error =
                    "Expected JSON array.";

                return false;
            }

            value.type =
                JsonValue::Type::Array;

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

                value.arrayValue.push_back(
                    std::move(child));

                skipWhitespace();

                if (position_ < input_.size() &&
                    input_[position_] == ']')
                {
                    ++position_;
                    return true;
                }

                if (!consume(','))
                {
                    error =
                        "Expected ',' in JSON array.";

                    return false;
                }
            }

            error =
                "Unterminated JSON array.";

            return false;
        }
    };

    bool getString(
        const JsonValue& object,
        const std::string& key,
        std::string& value,
        std::string& error)
    {
        const auto iterator =
            object.objectValue.find(key);

        if (iterator ==
            object.objectValue.end())
        {
            error =
                "Missing certificate field: " +
                key;

            return false;
        }

        if (iterator->second.type !=
            JsonValue::Type::String)
        {
            error =
                "Certificate field is not a string: " +
                key;

            return false;
        }

        value =
            iterator->second.stringValue;

        return true;
    }

    bool getScalar(
        const JsonValue& object,
        const std::string& key,
        std::string& value,
        std::string& error)
    {
        const auto iterator =
            object.objectValue.find(key);

        if (iterator ==
            object.objectValue.end())
        {
            error =
                "Missing certificate field: " +
                key;

            return false;
        }

        const JsonValue& child =
            iterator->second;

        if (child.type ==
                JsonValue::Type::String ||
            child.type ==
                JsonValue::Type::Number ||
            child.type ==
                JsonValue::Type::Boolean)
        {
            value = child.stringValue;
            return true;
        }

        error =
            "Certificate field is not scalar: " +
            key;

        return false;
    }

    std::string buildCanonicalCertificateData(
        const JsonValue& object,
        std::string& error)
    {
        std::string certificateId;
        std::string operationId;
        std::string requestId;
        std::string deviceId;
        std::string model;
        std::string serialNumber;
        std::string capacityBytes;
        std::string interfaceType;
        std::string method;
        std::string status;
        std::string bytesProcessed;
        std::string operationDurationMs;
        std::string verificationStatus;
        std::string verificationPerformed;
        std::string verificationPassed;
        std::string bytesVerified;
        std::string verificationSamples;
        std::string deviceReportedSuccess;
        std::string globalDataErased;
        std::string nativeErrorCode;
        std::string verificationMessage;
        std::string generatedAt;
        std::string hashAlgorithm;
        std::string message;

        if (!getString(
                object,
                "certificateId",
                certificateId,
                error) ||
            !getString(
                object,
                "operationId",
                operationId,
                error) ||
            !getString(
                object,
                "requestId",
                requestId,
                error) ||
            !getString(
                object,
                "deviceId",
                deviceId,
                error) ||
            !getString(
                object,
                "model",
                model,
                error) ||
            !getString(
                object,
                "serialNumber",
                serialNumber,
                error) ||
            !getScalar(
                object,
                "capacityBytes",
                capacityBytes,
                error) ||
            !getString(
                object,
                "interfaceType",
                interfaceType,
                error) ||
            !getString(
                object,
                "method",
                method,
                error) ||
            !getString(
                object,
                "status",
                status,
                error) ||
            !getScalar(
                object,
                "bytesProcessed",
                bytesProcessed,
                error) ||
            !getScalar(
                object,
                "operationDurationMs",
                operationDurationMs,
                error) ||
            !getString(
                object,
                "verificationStatus",
                verificationStatus,
                error) ||
            !getScalar(
                object,
                "verificationPerformed",
                verificationPerformed,
                error) ||
            !getScalar(
                object,
                "verificationPassed",
                verificationPassed,
                error) ||
            !getScalar(
                object,
                "bytesVerified",
                bytesVerified,
                error) ||
            !getScalar(
                object,
                "verificationSamples",
                verificationSamples,
                error) ||
            !getScalar(
                object,
                "deviceReportedSuccess",
                deviceReportedSuccess,
                error) ||
            !getScalar(
                object,
                "globalDataErased",
                globalDataErased,
                error) ||
            !getScalar(
                object,
                "nativeErrorCode",
                nativeErrorCode,
                error) ||
            !getString(
                object,
                "verificationMessage",
                verificationMessage,
                error) ||
            !getString(
                object,
                "generatedAt",
                generatedAt,
                error) ||
            !getString(
                object,
                "hashAlgorithm",
                hashAlgorithm,
                error) ||
            !getString(
                object,
                "message",
                message,
                error))
        {
            return {};
        }

        std::ostringstream canonical;

        canonical
            << "certificateId="
            << certificateId
            << '\n';

        canonical
            << "operationId="
            << operationId
            << '\n';

        canonical
            << "requestId="
            << requestId
            << '\n';

        canonical
            << "deviceId="
            << deviceId
            << '\n';

        canonical
            << "model="
            << model
            << '\n';

        canonical
            << "serialNumber="
            << serialNumber
            << '\n';

        canonical
            << "capacityBytes="
            << capacityBytes
            << '\n';

        canonical
            << "interfaceType="
            << interfaceType
            << '\n';

        canonical
            << "method="
            << method
            << '\n';

        canonical
            << "status="
            << status
            << '\n';

        canonical
            << "bytesProcessed="
            << bytesProcessed
            << '\n';

        canonical
            << "operationDurationMs="
            << operationDurationMs
            << '\n';

        canonical
            << "verificationStatus="
            << verificationStatus
            << '\n';

        canonical
            << "verificationPerformed="
            << verificationPerformed
            << '\n';

        canonical
            << "verificationPassed="
            << verificationPassed
            << '\n';

        canonical
            << "bytesVerified="
            << bytesVerified
            << '\n';

        canonical
            << "verificationSamples="
            << verificationSamples
            << '\n';

        canonical
            << "deviceReportedSuccess="
            << deviceReportedSuccess
            << '\n';

        canonical
            << "globalDataErased="
            << globalDataErased
            << '\n';

        canonical
            << "nativeErrorCode="
            << nativeErrorCode
            << '\n';

        canonical
            << "verificationMessage="
            << verificationMessage
            << '\n';

        canonical
            << "generatedAt="
            << generatedAt
            << '\n';

        canonical
            << "hashAlgorithm="
            << hashAlgorithm
            << '\n';

        canonical
            << "message="
            << message
            << '\n';

        return canonical.str();
    }
}

std::string CertificateVerifier::calculateSha256(
    const std::string& data)
{
    BCRYPT_ALG_HANDLE algorithmHandle = nullptr;
    BCRYPT_HASH_HANDLE hashHandle = nullptr;

    DWORD objectSize = 0;
    DWORD hashSize = 0;
    DWORD resultSize = 0;

    NTSTATUS status =
        BCryptOpenAlgorithmProvider(
            &algorithmHandle,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0);

    if (status < 0)
        throw std::runtime_error(
            "Failed to open BCrypt SHA-256 provider.");

    status =
        BCryptGetProperty(
            algorithmHandle,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(
                &objectSize),
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

    status =
        BCryptGetProperty(
            algorithmHandle,
            BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(
                &hashSize),
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

    status =
        BCryptCreateHash(
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

    status =
        BCryptHashData(
            hashHandle,
            reinterpret_cast<PUCHAR>(
                const_cast<char*>(
                    data.data())),
            static_cast<ULONG>(
                data.size()),
            0);

    if (status < 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        throw std::runtime_error(
            "Failed to hash certificate data.");
    }

    status =
        BCryptFinishHash(
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
            "Failed to finalize certificate hash.");
    }

    std::ostringstream stream;

    stream
        << std::hex
        << std::setfill('0');

    for (const UCHAR byte : hash)
    {
        stream
            << std::setw(2)
            << static_cast<unsigned int>(byte);
    }

    BCryptDestroyHash(hashHandle);
    BCryptCloseAlgorithmProvider(
        algorithmHandle,
        0);

    return stream.str();
}

CertificateVerificationResult
CertificateVerifier::verify(
    const std::filesystem::path& certificatePath) const
{
    CertificateVerificationResult result;

    if (certificatePath.empty())
    {
        result.message =
            "Certificate path is empty.";

        return result;
    }

    if (!std::filesystem::exists(
            certificatePath))
    {
        result.message =
            "Certificate file does not exist: " +
            certificatePath.string();

        return result;
    }

    std::string canonicalData;

    if (!loadCertificate(
            certificatePath,
            canonicalData,
            result))
    {
        return result;
    }

    try
    {
        result.calculatedHash =
            calculateSha256(
                canonicalData);
    }
    catch (const std::exception& exception)
    {
        result.message =
            "Certificate SHA-256 calculation failed: " +
            std::string(exception.what());

        return result;
    }

    result.hashMatched =
        result.calculatedHash ==
        result.storedHash;

    result.successfulSanitizationEvidence =
        result.requiredFieldsPresent &&
        result.hashMatched;

    result.valid =
        result.requiredFieldsPresent &&
        result.hashMatched;

    if (result.valid)
    {
        result.message =
            "Certificate structure and SHA-256 integrity verification passed.";
    }
    else if (!result.hashMatched)
    {
        result.message =
            "Certificate SHA-256 integrity verification failed.";
    }
    else
    {
        result.message =
            "Certificate required-field validation failed.";
    }

    return result;
}

bool CertificateVerifier::loadCertificate(
    const std::filesystem::path& certificatePath,
    std::string& canonicalData,
    CertificateVerificationResult& result)
{
    std::ifstream input(
        certificatePath,
        std::ios::in |
        std::ios::binary);

    if (!input)
    {
        result.message =
            "Unable to open certificate file: " +
            certificatePath.string();

        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    const std::string json =
        buffer.str();

    JsonValue root;
    std::string parseError;

    JsonParser parser(json);

    if (!parser.parse(
            root,
            parseError))
    {
        result.message =
            "Invalid certificate JSON: " +
            parseError;

        return false;
    }

    if (root.type !=
        JsonValue::Type::Object)
    {
        result.message =
            "Certificate root is not a JSON object.";

        return false;
    }

    std::string error;

    if (!getString(
            root,
            "certificateId",
            result.certificateId,
            error) ||
        !getString(
            root,
            "operationId",
            result.operationId,
            error) ||
        !getString(
            root,
            "deviceId",
            result.deviceId,
            error) ||
        !getString(
            root,
            "serialNumber",
            result.serialNumber,
            error) ||
        !getString(
            root,
            "certificateHash",
            result.storedHash,
            error))
    {
        result.message = error;
        return false;
    }

    std::string capacityValue;

    if (!getScalar(
            root,
            "capacityBytes",
            capacityValue,
            error))
    {
        result.message = error;
        return false;
    }

    try
    {
        result.capacityBytes =
            std::stoull(capacityValue);
    }
    catch (...)
    {
        result.message =
            "Invalid certificate capacityBytes.";

        return false;
    }

    /*
     * Required successful-operation fields.
     *
     * These checks mirror the current SanitizationCertificate
     * validity requirements without invoking that implementation.
     */
    std::string status;
    std::string verificationStatus;
    std::string verificationPerformed;
    std::string verificationPassed;

    if (!getString(
            root,
            "status",
            status,
            error) ||
        !getString(
            root,
            "verificationStatus",
            verificationStatus,
            error) ||
        !getScalar(
            root,
            "verificationPerformed",
            verificationPerformed,
            error) ||
        !getScalar(
            root,
            "verificationPassed",
            verificationPassed,
            error))
    {
        result.message = error;
        return false;
    }

    result.requiredFieldsPresent =
        !result.certificateId.empty() &&
        !result.operationId.empty() &&
        !result.deviceId.empty() &&
        !result.serialNumber.empty() &&
        result.capacityBytes > 0 &&
        !result.storedHash.empty();

    result.successfulSanitizationEvidence =
        status == "COMPLETED" &&
        verificationStatus == "PASSED" &&
        verificationPerformed == "true" &&
        verificationPassed == "true";

    canonicalData =
        buildCanonicalCertificateData(
            root,
            error);

    if (!error.empty())
    {
        result.message =
            "Unable to reconstruct certificate canonical data: " +
            error;

        return false;
    }

    return true;
}