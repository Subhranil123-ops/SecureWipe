#include "CertificateGenerator.h"

#include <Windows.h>
#include <bcrypt.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

using namespace SecureWipe;

namespace
{
    std::string toString(SanitizationMethod method)
    {
        switch (method)
        {
        case SanitizationMethod::HostOverwrite:
            return "HOST_OVERWRITE";
        case SanitizationMethod::AtaSanitize:
            return "ATA_SANITIZE";
        case SanitizationMethod::NvmeSanitize:
            return "NVME_SANITIZE";
        case SanitizationMethod::Unsupported:
        default:
            return "UNSUPPORTED";
        }
    }

    std::string toString(SanitizationStatus status)
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
        default:
            return "UNKNOWN";
        }
    }

    std::string toString(VerificationStatus status)
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
        default:
            return "UNKNOWN";
        }
    }

    std::string boolToString(bool value)
    {
        return value ? "true" : "false";
    }
}

SanitizationCertificate CertificateGenerator::generate(
    const SanitizationResult &result,
    const std::string &requestId) const
{
    SanitizationCertificate certificate;

    certificate.certificateId = generateCertificateId(result);
    certificate.operationId = result.operationId;
    certificate.requestId = requestId;

    certificate.deviceId = result.deviceId;
    certificate.model = result.model;
    certificate.serialNumber = result.serialNumber;
    certificate.capacityBytes = result.capacityBytes;
    certificate.interfaceType = result.interfaceType;

    certificate.method = result.method;
    certificate.status = result.status;
    certificate.bytesProcessed = result.bytesProcessed;
    certificate.operationDurationMs = result.operationDurationMs;

    certificate.verificationStatus = result.verificationStatus;
    certificate.verificationPerformed = result.verificationPerformed;
    certificate.verificationPassed =
        result.verificationPerformed &&
        result.verificationStatus == VerificationStatus::PASSED;

    certificate.bytesVerified = result.bytesVerified;
    certificate.verificationSamples = result.verificationSamples;

    certificate.deviceReportedSuccess = false;
    certificate.globalDataErased = false;

    certificate.nativeErrorCode = result.nativeErrorCode;
    certificate.verificationMessage = result.verificationMessage;

    certificate.generatedAt = generateTimestamp();
    certificate.hashAlgorithm = "SHA-256";

    if (certificate.isValid())
    {
        certificate.message =
            "Sanitization completed successfully and verification passed.";
    }
    else
    {
        certificate.message =
            "Certificate generated from sanitization evidence, but the "
            "operation is not eligible for a successful sanitization certificate.";
    }

    const std::string canonicalData = buildCanonicalData(certificate);
    certificate.certificateHash = calculateSha256(canonicalData);

    return certificate;
}

std::string CertificateGenerator::generateCertificateId(
    const SanitizationResult &result) const
{
    const auto now = std::chrono::system_clock::now();

    const auto timestamp =
        std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch())
            .count();

    std::ostringstream stream;

    stream << "SW-CERT-" << timestamp;

    if (!result.operationId.empty())
        stream << "-" << result.operationId;

    return stream.str();
}

std::string CertificateGenerator::generateTimestamp() const
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t timeValue =
        std::chrono::system_clock::to_time_t(now);

    std::tm utcTime{};

#ifdef _WIN32
    gmtime_s(&utcTime, &timeValue);
#else
    gmtime_r(&timeValue, &utcTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");

    return stream.str();
}

std::string CertificateGenerator::buildCanonicalData(
    const SanitizationCertificate &certificate) const
{
    std::ostringstream stream;

    stream << "certificateId=" << certificate.certificateId << '\n';
    stream << "operationId=" << certificate.operationId << '\n';
    stream << "requestId=" << certificate.requestId << '\n';

    stream << "deviceId=" << certificate.deviceId << '\n';
    stream << "model=" << certificate.model << '\n';
    stream << "serialNumber=" << certificate.serialNumber << '\n';
    stream << "capacityBytes=" << certificate.capacityBytes << '\n';
    stream << "interfaceType=" << certificate.interfaceType << '\n';

    stream << "method=" << toString(certificate.method) << '\n';
    stream << "status=" << toString(certificate.status) << '\n';
    stream << "bytesProcessed=" << certificate.bytesProcessed << '\n';
    stream << "operationDurationMs="
           << certificate.operationDurationMs << '\n';

    stream << "verificationStatus="
           << toString(certificate.verificationStatus) << '\n';

    stream << "verificationPerformed="
           << boolToString(certificate.verificationPerformed) << '\n';

    stream << "verificationPassed="
           << boolToString(certificate.verificationPassed) << '\n';

    stream << "bytesVerified="
           << certificate.bytesVerified << '\n';

    stream << "verificationSamples="
           << certificate.verificationSamples << '\n';

    stream << "deviceReportedSuccess="
           << boolToString(certificate.deviceReportedSuccess) << '\n';

    stream << "globalDataErased="
           << boolToString(certificate.globalDataErased) << '\n';

    stream << "nativeErrorCode="
           << certificate.nativeErrorCode << '\n';

    stream << "verificationMessage="
           << certificate.verificationMessage << '\n';

    stream << "generatedAt="
           << certificate.generatedAt << '\n';

    stream << "hashAlgorithm="
           << certificate.hashAlgorithm << '\n';

    stream << "message="
           << certificate.message << '\n';

    return stream.str();
}

std::string CertificateGenerator::calculateSha256(
    const std::string &data) const
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
    {
        throw std::runtime_error(
            "Failed to open Windows BCrypt SHA-256 provider.");
    }

    status = BCryptGetProperty(
        algorithmHandle,
        BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectSize),
        sizeof(objectSize),
        &resultSize,
        0);

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);

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
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);

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
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);

        throw std::runtime_error(
            "Failed to create BCrypt SHA-256 hash.");
    }

    status = BCryptHashData(
        hashHandle,
        reinterpret_cast<PUCHAR>(
            const_cast<char *>(data.data())),
        static_cast<ULONG>(data.size()),
        0);

    if (status < 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);

        throw std::runtime_error(
            "Failed to hash certificate data.");
    }

    status = BCryptFinishHash(
        hashHandle,
        hash.data(),
        hashSize,
        0);

    if (status < 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);

        throw std::runtime_error(
            "Failed to finalize SHA-256 hash.");
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0');

    for (const UCHAR byte : hash)
    {
        stream << std::setw(2)
               << static_cast<unsigned int>(byte);
    }

    BCryptDestroyHash(hashHandle);
    BCryptCloseAlgorithmProvider(algorithmHandle, 0);

    return stream.str();
}