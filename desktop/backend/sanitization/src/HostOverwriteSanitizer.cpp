#include "HostOverwriteSanitizer.h"

#include <algorithm>
#include <iostream>
#include <vector>

VerificationResult HostOverwriteSanitizer::verify(
    HANDLE deviceHandle,
    std::uint64_t totalBytes)
{
    VerificationResult result;

    constexpr std::size_t VERIFY_SIZE = 4096;

    result.performed = true;

    std::vector<std::uint8_t> buffer(VERIFY_SIZE);

    const std::uint64_t offsets[] = {
        0,
        totalBytes / 4,
        totalBytes / 2,
        (totalBytes * 3) / 4,
        totalBytes > VERIFY_SIZE ? totalBytes - VERIFY_SIZE : 0};

    for (std::uint64_t offset : offsets)
    {
        LARGE_INTEGER position;
        position.QuadPart = static_cast<LONGLONG>(offset);

        if (!SetFilePointerEx(
                deviceHandle,
                position,
                nullptr,
                FILE_BEGIN))
        {
            result.passed = false;
            result.message =
                "Verification failed: unable to seek to verification offset.";
            return result;
        }

        DWORD bytesRead = 0;

        if (!ReadFile(
                deviceHandle,
                buffer.data(),
                VERIFY_SIZE,
                &bytesRead,
                nullptr))
        {
            result.passed = false;
            result.message =
                "Verification failed: unable to read verification data.";
            return result;
        }

        if (bytesRead != VERIFY_SIZE)
        {
            result.passed = false;
            result.message =
                "Verification failed: incomplete verification read.";
            return result;
        }

        if (!std::all_of(
                buffer.begin(),
                buffer.end(),
                [](std::uint8_t value)
                {
                    return value == 0x00;
                }))
        {
            result.passed = false;
            result.message =
                "Verification failed: non-zero data detected.";
            return result;
        }

        result.bytesVerified += bytesRead;
        result.samples++;
    }

    result.passed = true;
    result.message =
        "Host overwrite verification passed.";

    std::cout << result.message << '\n';

    return result;
}

bool HostOverwriteSanitizer::overwrite(
    HANDLE deviceHandle,
    std::uint64_t totalBytes)
{
    std::vector<std::uint8_t> buffer(
        BUFFER_SIZE,
        0x00);

    std::uint64_t offset = 0;

    while (offset < totalBytes)
    {
        const DWORD bytesToWrite =
            static_cast<DWORD>(
                std::min<std::uint64_t>(
                    BUFFER_SIZE,
                    totalBytes - offset));

        LARGE_INTEGER position;
        position.QuadPart =
            static_cast<LONGLONG>(offset);

        if (!SetFilePointerEx(
                deviceHandle,
                position,
                nullptr,
                FILE_BEGIN))
        {
            return false;
        }

        DWORD bytesWritten = 0;

        if (!WriteFile(
                deviceHandle,
                buffer.data(),
                bytesToWrite,
                &bytesWritten,
                nullptr))
        {
            return false;
        }

        if (bytesWritten != bytesToWrite)
        {
            return false;
        }

        offset += bytesWritten;

        const int progress =
            static_cast<int>(
                (offset * 100) / totalBytes);

        std::cout
            << "\rHost overwrite: "
            << progress
            << "%"
            << std::flush;
    }

    std::cout
        << "\nHost overwrite completed.\n";

    return true;
}

VerificationResult HostOverwriteSanitizer::sanitize(
    HANDLE deviceHandle,
    std::uint64_t totalBytes)
{
    VerificationResult result;

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.message =
            "Sanitization failed: invalid device handle.";
        return result;
    }

    if (totalBytes == 0)
    {
        result.message =
            "Sanitization failed: device capacity is zero.";
        return result;
    }

    if (!overwrite(deviceHandle, totalBytes))
    {
        result.message =
            "Sanitization failed: host overwrite failed.";
        return result;
    }

    if (!FlushFileBuffers(deviceHandle))
    {
        result.message =
            "Sanitization failed: unable to flush device buffers.";
        return result;
    }

    return verify(deviceHandle, totalBytes);
}