#pragma once

#include <filesystem>
#include <mutex>
#include <string>

#include "SanitizationEvent.h"

class AuditLogger
{
public:
    explicit AuditLogger(
        const std::filesystem::path& logFilePath);

    bool append(
        SecureWipe::SanitizationEvent event,
        std::string& errorMessage);

    const std::filesystem::path& logFilePath() const;

private:
    std::filesystem::path logFilePath_;
    std::string previousEventHash_;
    std::mutex mutex_;

    bool loadLastEventHash(
        std::string& errorMessage);

    static std::string calculateSha256(
        const std::string& data);
};