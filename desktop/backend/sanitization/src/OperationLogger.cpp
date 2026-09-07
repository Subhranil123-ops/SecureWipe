#include "OperationLogger.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace
{
std::string timestampUtc()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(now);
    std::tm utcTime{};

#ifdef _WIN32
    gmtime_s(&utcTime, &timeValue);
#else
    gmtime_r(&timeValue, &utcTime);
#endif

    std::ostringstream output;
    output << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}
}

OperationLogger::OperationLogger(const std::filesystem::path& logFilePath)
    : logFilePath_(logFilePath)
{
}

const std::filesystem::path& OperationLogger::logFilePath() const
{
    return logFilePath_;
}

bool OperationLogger::info(const std::string& message, std::string& errorMessage)
{
    return write("INFO", message, errorMessage);
}

bool OperationLogger::warning(const std::string& message, std::string& errorMessage)
{
    return write("WARNING", message, errorMessage);
}

bool OperationLogger::error(const std::string& message, std::string& errorMessage)
{
    return write("ERROR", message, errorMessage);
}

bool OperationLogger::write(const char* level, const std::string& message, std::string& errorMessage)
{
    std::lock_guard<std::mutex> lock(mutex_);

    try
    {
        const auto parent = logFilePath_.parent_path();

        if (!parent.empty())
            std::filesystem::create_directories(parent);

        std::ofstream output(logFilePath_, std::ios::out | std::ios::app);

        if (!output)
        {
            errorMessage = "Unable to open operation log: " + logFilePath_.string();
            return false;
        }

        output << timestampUtc()
               << " ["
               << level
               << "] "
               << message
               << '\n';

        output.flush();

        if (!output.good())
        {
            errorMessage = "Unable to write operation log.";
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