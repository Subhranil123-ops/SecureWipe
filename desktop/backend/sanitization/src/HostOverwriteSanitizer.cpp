#include "HostOverwriteSanitizer.h"

#include <algorithm>
#include <iostream>
#include <vector>
#include <sstream>

namespace
{
std::string winErrorMessage(DWORD errorCode)
{
    if (errorCode == ERROR_SUCCESS)
        return "ERROR_SUCCESS";

    LPSTR buffer = nullptr;

    const DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS;

    const DWORD length =
        FormatMessageA(
            flags,
            nullptr,
            errorCode,
            0,
            reinterpret_cast<LPSTR>(&buffer),
            0,
            nullptr);

    std::string message;

    if (length != 0 && buffer != nullptr)
    {
        message.assign(buffer, length);

        while (!message.empty() &&
               (message.back() == '\r' ||
                message.back() == '\n' ||
                message.back() == ' '))
        {
            message.pop_back();
        }
    }
    else
    {
        message = "Unknown Windows error";
    }

    if (buffer != nullptr)
        LocalFree(buffer);

    return message;
}

std::string buildIoError(
    const std::string& phase,
    DWORD errorCode,
    std::uint64_t offset,
    DWORD requested,
    DWORD actual)
{
    std::ostringstream stream;

    stream
        << phase
        << " failed."
        << " Windows error=" << errorCode
        << " (" << winErrorMessage(errorCode) << ")"
        << ", offset=" << offset
        << ", requested=" << requested
        << ", actual=" << actual;

    return stream.str();
}
}

bool HostOverwriteSanitizer::getSectorSize(
    HANDLE deviceHandle,
    std::uint32_t& sectorSize)
{
    sectorSize = 512;

    DISK_GEOMETRY_EX geometry{};
    DWORD returnedBytes = 0;

    if (DeviceIoControl(
            deviceHandle,
            IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            nullptr,
            0,
            &geometry,
            sizeof(geometry),
            &returnedBytes,
            nullptr))
    {
        if (geometry.Geometry.BytesPerSector != 0)
        {
            sectorSize =
                geometry.Geometry.BytesPerSector;
        }
    }

    return sectorSize != 0;
}

bool HostOverwriteSanitizer::checkWritable(
    HANDLE deviceHandle,
    VerificationResult& result)
{
    if (DeviceIoControl(
            deviceHandle,
            IOCTL_DISK_IS_WRITABLE,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            nullptr))
    {
        return true;
    }

    const DWORD errorCode = GetLastError();

    // Some storage bridges may not implement this control code.
    // In that case, allow the actual WriteFile call to determine
    // writability and capture the real error there.
    if (errorCode == ERROR_INVALID_FUNCTION ||
        errorCode == ERROR_NOT_SUPPORTED)
    {
        std::cout
            << "Writable preflight is not supported by this device."
            << " Continuing to actual write test.\n";

        return true;
    }

    result.nativeErrorCode = errorCode;
    result.phase = "Writable preflight";
    result.message =
        "Target device is not writable: Windows error=" +
        std::to_string(errorCode) +
        " (" +
        winErrorMessage(errorCode) +
        ").";

    return false;
}

bool HostOverwriteSanitizer::overwrite(
    HANDLE deviceHandle,
    std::uint64_t totalBytes,
    std::uint32_t sectorSize,
    VerificationResult& result)
{
    std::size_t chunkSize = BUFFER_SIZE;

    if (sectorSize > 0)
    {
        chunkSize =
            (chunkSize / sectorSize) * sectorSize;
    }

    if (chunkSize < sectorSize)
        chunkSize = sectorSize;

    std::vector<std::uint8_t> buffer(
        chunkSize,
        0x00);

    LARGE_INTEGER startPosition{};
    startPosition.QuadPart = 0;

    if (!SetFilePointerEx(
            deviceHandle,
            startPosition,
            nullptr,
            FILE_BEGIN))
    {
        const DWORD errorCode = GetLastError();

        result.nativeErrorCode = errorCode;
        result.failedOffset = 0;
        result.phase = "Initial seek";
        result.message =
            buildIoError(
                "Initial seek",
                errorCode,
                0,
                0,
                0);

        return false;
    }

    std::uint64_t offset = 0;

    while (offset < totalBytes)
    {
        const std::uint64_t remaining =
            totalBytes - offset;

        DWORD bytesToWrite =
            static_cast<DWORD>(
                std::min<std::uint64_t>(
                    chunkSize,
                    remaining));

        // Every write except the final one should be sector aligned.
        if (remaining > bytesToWrite)
        {
            bytesToWrite =
                (bytesToWrite / sectorSize) *
                sectorSize;
        }

        if (bytesToWrite == 0)
        {
            result.nativeErrorCode =
                ERROR_INVALID_PARAMETER;

            result.failedOffset = offset;
            result.phase = "Write preparation";
            result.message =
                "Unable to create a sector-aligned write request.";

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
            const DWORD errorCode = GetLastError();

            result.nativeErrorCode = errorCode;
            result.failedOffset = offset;
            result.requestedBytes = bytesToWrite;
            result.actualBytes = bytesWritten;
            result.phase = "WriteFile";
            result.message =
                buildIoError(
                    "WriteFile",
                    errorCode,
                    offset,
                    bytesToWrite,
                    bytesWritten);

            return false;
        }

        if (bytesWritten == 0)
        {
            result.nativeErrorCode =
                ERROR_WRITE_FAULT;

            result.failedOffset = offset;
            result.requestedBytes = bytesToWrite;
            result.actualBytes = 0;
            result.phase = "WriteFile";
            result.message =
                "WriteFile returned success but wrote zero bytes.";

            return false;
        }

        if (bytesWritten != bytesToWrite)
        {
            result.nativeErrorCode =
                ERROR_WRITE_FAULT;

            result.failedOffset = offset;
            result.requestedBytes = bytesToWrite;
            result.actualBytes = bytesWritten;
            result.phase = "Partial write";
            result.message =
                buildIoError(
                    "Partial write",
                    ERROR_WRITE_FAULT,
                    offset,
                    bytesToWrite,
                    bytesWritten);

            return false;
        }

        offset += bytesWritten;

        const int progress =
            static_cast<int>(
                (offset * 100ULL) /
                totalBytes);

        std::cout
            << "\rHost overwrite: "
            << progress
            << "% "
            << std::flush;
    }

    std::cout
        << "\nHost overwrite completed.\n";

    result.deviceReportedSuccess = true;

    return true;
}

VerificationResult HostOverwriteSanitizer::verify(
    HANDLE deviceHandle,
    std::uint64_t totalBytes,
    std::uint32_t sectorSize)
{
    VerificationResult result;

    result.method =
        VerificationMethod::HOST_READ_BACK;

    result.performed = true;

    if (totalBytes < VERIFY_SIZE)
    {
        result.performed = false;
        result.message =
            "Verification failed: device is smaller than verification block.";
        return result;
    }

    std::vector<std::uint8_t> buffer(
        VERIFY_SIZE);

    std::uint64_t offsets[] = {
        0,
        totalBytes / 4,
        totalBytes / 2,
        (totalBytes * 3) / 4,
        totalBytes - VERIFY_SIZE
    };

    for (std::uint64_t offset : offsets)
    {
        // Keep verification offsets sector aligned.
        if (sectorSize != 0)
        {
            offset =
                (offset / sectorSize) *
                sectorSize;
        }

        if (offset + VERIFY_SIZE > totalBytes)
        {
            offset =
                totalBytes - VERIFY_SIZE;
        }

        LARGE_INTEGER position{};
        position.QuadPart =
            static_cast<LONGLONG>(offset);

        if (!SetFilePointerEx(
                deviceHandle,
                position,
                nullptr,
                FILE_BEGIN))
        {
            const DWORD errorCode = GetLastError();

            result.nativeErrorCode = errorCode;
            result.failedOffset = offset;
            result.phase = "Verification seek";
            result.message =
                buildIoError(
                    "Verification seek",
                    errorCode,
                    offset,
                    VERIFY_SIZE,
                    0);

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
            const DWORD errorCode = GetLastError();

            result.nativeErrorCode = errorCode;
            result.failedOffset = offset;
            result.requestedBytes = VERIFY_SIZE;
            result.actualBytes = bytesRead;
            result.phase = "Verification read";
            result.message =
                buildIoError(
                    "Verification read",
                    errorCode,
                    offset,
                    VERIFY_SIZE,
                    bytesRead);

            return result;
        }

        if (bytesRead != VERIFY_SIZE)
        {
            result.nativeErrorCode =
                ERROR_READ_FAULT;

            result.failedOffset = offset;
            result.requestedBytes = VERIFY_SIZE;
            result.actualBytes = bytesRead;
            result.phase = "Verification read";

            result.message =
                buildIoError(
                    "Incomplete verification read",
                    ERROR_READ_FAULT,
                    offset,
                    VERIFY_SIZE,
                    bytesRead);

            return result;
        }

        const bool allZero =
            std::all_of(
                buffer.begin(),
                buffer.end(),
                [](std::uint8_t value)
                {
                    return value == 0x00;
                });

        if (!allZero)
        {
            result.passed = false;
            result.failedOffset = offset;
            result.phase = "Verification data check";
            result.message =
                "Verification failed: non-zero data detected at sampled offset.";

            return result;
        }

        result.bytesVerified +=
            bytesRead;

        result.samples++;
    }

    result.passed = true;
    result.globalDataErased = true;

    result.message =
        "Host overwrite verification passed: all sampled regions contain 0x00.";

    std::cout
        << result.message
        << '\n';

    return result;
}

VerificationResult HostOverwriteSanitizer::sanitize(
    HANDLE deviceHandle,
    std::uint64_t totalBytes)
{
    VerificationResult result;

    result.method =
        VerificationMethod::HOST_READ_BACK;

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.message =
            "Sanitization failed: invalid device handle.";
        result.phase = "Handle validation";
        return result;
    }

    if (totalBytes == 0)
    {
        result.message =
            "Sanitization failed: device capacity is zero.";
        result.phase = "Capacity validation";
        return result;
    }

    std::uint32_t sectorSize = 512;

    if (!getSectorSize(
            deviceHandle,
            sectorSize))
    {
        result.nativeErrorCode =
            ERROR_INVALID_DATA;

        result.phase =
            "Sector size detection";

        result.message =
            "Unable to determine a valid physical sector size.";

        return result;
    }

    std::cout
        << "Physical sector size: "
        << sectorSize
        << " bytes\n";

    if ((totalBytes % sectorSize) != 0)
    {
        result.nativeErrorCode =
            ERROR_INVALID_DATA;

        result.phase =
            "Capacity alignment";

        result.message =
            "Device capacity is not aligned to the physical sector size.";

        return result;
    }

    if (!checkWritable(
            deviceHandle,
            result))
    {
        return result;
    }

    std::cout
        << "Writable preflight passed.\n";

    if (!overwrite(
            deviceHandle,
            totalBytes,
            sectorSize,
            result))
    {
        return result;
    }

    std::cout
        << "Flushing device buffers...\n";

    if (!FlushFileBuffers(deviceHandle))
    {
        const DWORD errorCode = GetLastError();

        result.nativeErrorCode = errorCode;
        result.phase = "FlushFileBuffers";
        result.message =
            "FlushFileBuffers failed: Windows error=" +
            std::to_string(errorCode) +
            " (" +
            winErrorMessage(errorCode) +
            ").";

        return result;
    }

    std::cout
        << "Device buffers flushed successfully.\n";

    return verify(
        deviceHandle,
        totalBytes,
        sectorSize);
}