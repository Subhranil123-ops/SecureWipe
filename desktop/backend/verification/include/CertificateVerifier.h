#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

struct CertificateVerificationResult
{
    bool valid = false;

    std::string certificateId;
    std::string operationId;
    std::string storedHash;
    std::string calculatedHash;

    std::string deviceId;
    std::string serialNumber;

    std::uint64_t capacityBytes = 0;

    std::string message;

    bool hashMatched = false;
    bool requiredFieldsPresent = false;
    bool successfulSanitizationEvidence = false;

    bool isSuccess() const
    {
        return valid &&
               hashMatched &&
               requiredFieldsPresent &&
               successfulSanitizationEvidence;
    }
};

class CertificateVerifier
{
public:
    CertificateVerifier() = default;

    CertificateVerificationResult verify(
        const std::filesystem::path& certificatePath) const;

private:
    static std::string calculateSha256(
        const std::string& data);

    static bool loadCertificate(
        const std::filesystem::path& certificatePath,
        std::string& canonicalData,
        CertificateVerificationResult& result);
};