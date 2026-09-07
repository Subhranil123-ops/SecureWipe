#include "AuditLogger.h"

#ifdef _WIN32
#include <Windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

AuditLogger::AuditLogger(
    const std::filesystem::path& logFilePath)
    : logFilePath_(logFilePath)
{
    std::string ignored;
    loadLastEventHash(ignored);
}

const std::filesystem::path&
AuditLogger::logFilePath() const
{
    return logFilePath_;
}

bool AuditLogger::append(
    SecureWipe::SanitizationEvent event,
    std::string& errorMessage)
{
    std::lock_guard<std::mutex> lock(mutex_);

    try
    {
        const auto parent =
            logFilePath_.parent_path();

        if (!parent.empty())
            std::filesystem::create_directories(parent);

        if (event.eventId.empty())
        {
            event.eventId =
                SecureWipe::SanitizationEvent::
                    generateEventId();
        }

        if (event.timestampUtc.empty())
        {
            event.timestampUtc =
                SecureWipe::SanitizationEvent::
                    generateTimestampUtc();
        }

        event.previousEventHash =
            previousEventHash_;

        event.eventHash =
            calculateSha256(
                event.canonicalData());

        std::ofstream output(
            logFilePath_,
            std::ios::out |
            std::ios::app |
            std::ios::binary);

        if (!output)
        {
            errorMessage =
                "Unable to open audit log: " +
                logFilePath_.string();

            return false;
        }

        output << event.toJsonLine()
               << '\n';

        output.flush();

        if (!output.good())
        {
            errorMessage =
                "Unable to write audit event: " +
                logFilePath_.string();

            return false;
        }

        previousEventHash_ =
            event.eventHash;

        return true;
    }
    catch (const std::exception& exception)
    {
        errorMessage = exception.what();
        return false;
    }
}

bool AuditLogger::loadLastEventHash(
    std::string& errorMessage)
{
    try
    {
        previousEventHash_.clear();

        if (!std::filesystem::exists(logFilePath_))
            return true;

        std::ifstream input(
            logFilePath_,
            std::ios::in |
            std::ios::binary);

        if (!input)
        {
            errorMessage =
                "Unable to open existing audit log.";

            return false;
        }

        std::string line;
        std::string lastLine;

        while (std::getline(input, line))
        {
            if (!line.empty())
                lastLine = line;
        }

        const std::string marker =
            "\"eventHash\":\"";

        const std::size_t start =
            lastLine.find(marker);

        if (start == std::string::npos)
            return true;

        const std::size_t valueStart =
            start + marker.size();

        const std::size_t end =
            lastLine.find(
                '"',
                valueStart);

        if (end == std::string::npos)
            return true;

        previousEventHash_ =
            lastLine.substr(
                valueStart,
                end - valueStart);

        return true;
    }
    catch (const std::exception& exception)
    {
        errorMessage = exception.what();
        return false;
    }
}

std::string AuditLogger::calculateSha256(
    const std::string& data)
{
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithmHandle =
        nullptr;

    BCRYPT_HASH_HANDLE hashHandle =
        nullptr;

    std::string result;

    if (BCryptOpenAlgorithmProvider(
            &algorithmHandle,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0) != 0)
    {
        return {};
    }

    DWORD objectSize = 0;
    DWORD resultSize = 0;

    if (BCryptGetProperty(
            algorithmHandle,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&objectSize),
            sizeof(objectSize),
            &resultSize,
            0) != 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    std::vector<UCHAR> objectBuffer(
        objectSize);

    // BCrypt does not define a digest-length constant for SHA-256 in all
    // Windows SDK versions.
    constexpr ULONG sha256DigestLength = 32;
    std::vector<UCHAR> hashBuffer(
        sha256DigestLength);

    if (BCryptCreateHash(
            algorithmHandle,
            &hashHandle,
            objectBuffer.data(),
            objectSize,
            nullptr,
            0,
            0) != 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    if (BCryptHashData(
            hashHandle,
            reinterpret_cast<PUCHAR>(
                const_cast<char*>(
                    data.data())),
            static_cast<ULONG>(data.size()),
            0) != 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    if (BCryptFinishHash(
            hashHandle,
            hashBuffer.data(),
            static_cast<ULONG>(
                hashBuffer.size()),
            0) != 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    BCryptDestroyHash(hashHandle);
    BCryptCloseAlgorithmProvider(
        algorithmHandle,
        0);

    std::ostringstream output;

    output << std::hex
           << std::setfill('0');

    for (const UCHAR byte : hashBuffer)
    {
        output << std::setw(2)
               << static_cast<unsigned int>(
                      byte);
    }

    return output.str();
#else
    (void)data;
    return {};
#endif
}