#pragma once

#include <filesystem>
#include <mutex>
#include <string>

class OperationLogger
{
public:
    explicit OperationLogger(const std::filesystem::path& logFilePath);

    bool info(const std::string& message, std::string& errorMessage);
    bool warning(const std::string& message, std::string& errorMessage);
    bool error(const std::string& message, std::string& errorMessage);

    const std::filesystem::path& logFilePath() const;

private:
    std::filesystem::path logFilePath_;
    std::mutex mutex_;

    bool write(const char* level, const std::string& message, std::string& errorMessage);
};