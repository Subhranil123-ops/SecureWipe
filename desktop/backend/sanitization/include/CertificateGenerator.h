#pragma once

#include <string>

#include "SanitizationCertificate.h"
#include "SanitizationResult.h"

class CertificateGenerator
{
public:
    CertificateGenerator() = default;

    SecureWipe::SanitizationCertificate generate(
        const SecureWipe::SanitizationResult& result,
        const std::string& requestId = "",
        const std::string& workstationId = "") const;

private:
    std::string generateCertificateId(
        const SecureWipe::SanitizationResult& result) const;

    std::string generateTimestamp() const;

    std::string buildCanonicalData(
        const SecureWipe::SanitizationCertificate& certificate) const;

    std::string calculateSha256(
        const std::string& data) const;
};