#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

struct AuditChainVerificationResult
{
    bool valid = false;
    std::size_t eventCount = 0;
    std::size_t verifiedEventCount = 0;
    std::size_t failedEventIndex = 0;
    std::string firstEventPreviousHash;
    std::string expectedHash;
    std::string actualHash;
    std::string message;

    bool isSuccess() const
    {
        return valid;
    }
};

class AuditChainVerifier
{
public:
    AuditChainVerifier() = default;

    AuditChainVerificationResult verify(
        const std::filesystem::path& auditLogPath) const;

private:
    static std::string calculateSha256(
        const std::string& data);

    static bool verifyEventChain(
        const std::filesystem::path& auditLogPath,
        AuditChainVerificationResult& result);
};