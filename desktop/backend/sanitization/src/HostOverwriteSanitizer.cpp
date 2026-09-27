#include "HostOverwriteSanitizer.h"

#include <Windows.h>
#include <winioctl.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string getWindowsErrorMessage(DWORD errorCode)
{
    if (errorCode == ERROR_SUCCESS)
        return "ERROR_SUCCESS";

    LPSTR buffer = nullptr;

    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);

    std::string message;

    if (length != 0 && buffer != nullptr)
    {
        message.assign(
            buffer,
            length);

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
        message =
            "Unknown Windows error";
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
        << " failed. Windows error="
        << errorCode
        << " ("
        << getWindowsErrorMessage(errorCode)
        << "), offset="
        << offset
        << ", requested="
        << requested
        << ", actual="
        << actual;

    return stream.str();
}

bool isPhysicalDeviceHandle(
    HANDLE deviceHandle)
{
    return deviceHandle != INVALID_HANDLE_VALUE;
}
}

bool HostOverwriteSanitizer::getSectorSize(
    HANDLE deviceHandle,
    std::uint32_t& sectorSize)
{
    sectorSize = 0;

    if (!isPhysicalDeviceHandle(deviceHandle))
        return false;

    STORAGE_PROPERTY_QUERY query{};

    query.PropertyId =
        StorageAccessAlignmentProperty;

    query.QueryType =
        PropertyStandardQuery;

    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR descriptor{};

    DWORD returnedBytes = 0;

    const BOOL success =
        DeviceIoControl(
            deviceHandle,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            &descriptor,
            sizeof(descriptor),
            &returnedBytes,
            nullptr);

    if (!success)
        return false;

    if (returnedBytes <
        sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR))
    {
        return false;
    }

    if (descriptor.BytesPerPhysicalSector != 0)
    {
        sectorSize =
            descriptor.BytesPerPhysicalSector;
    }
    else if (descriptor.BytesPerLogicalSector != 0)
    {
        sectorSize =
            descriptor.BytesPerLogicalSector;
    }

    if (sectorSize == 0)
        return false;

    return true;
}

bool HostOverwriteSanitizer::checkWritable(
    HANDLE deviceHandle,
    VerificationResult& result)
{
    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.phase =
            "Writable preflight";

        result.message =
            "Invalid device handle.";

        return false;
    }

    LARGE_INTEGER currentPosition{};

    if (!SetFilePointerEx(
            deviceHandle,
            LARGE_INTEGER{},
            &currentPosition,
            FILE_CURRENT))
    {
        const DWORD errorCode =
            GetLastError();

        result.nativeErrorCode =
            errorCode;

        result.phase =
            "Writable preflight";

        result.message =
            "Unable to query physical device file position. Windows error=" +
            std::to_string(errorCode) +
            " (" +
            getWindowsErrorMessage(errorCode) +
            ").";

        return false;
    }

    return true;
}

bool HostOverwriteSanitizer::lockTargetVolumes(
    HANDLE deviceHandle,
    std::vector<HANDLE>& lockedVolumes,
    VerificationResult& result)
{
    lockedVolumes.clear();

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.phase =
            "Volume preparation";

        result.message =
            "Invalid physical device handle.";

        return false;
    }

    std::cout
        << "\nPreparing target volumes...\n";

    // The physical device itself is the target. We deliberately
    // enumerate mounted volumes and try to lock/dismount only those
    // volumes which belong to the target physical device.
    //
    // Failure to identify an associated mounted volume is not treated
    // as proof that the physical device is unsafe. The SafetyEngine
    // remains responsible for the higher-level safety checks.

    DWORD returnedBytes = 0;

    STORAGE_DEVICE_NUMBER deviceNumber{};

    if (!DeviceIoControl(
            deviceHandle,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            nullptr,
            0,
            &deviceNumber,
            sizeof(deviceNumber),
            &returnedBytes,
            nullptr))
    {
        const DWORD errorCode =
            GetLastError();

        result.nativeErrorCode =
            errorCode;

        result.phase =
            "Volume preparation";

        result.message =
            "Unable to determine target storage device number. Windows error=" +
            std::to_string(errorCode) +
            " (" +
            getWindowsErrorMessage(errorCode) +
            ").";

        return false;
    }

    HANDLE findHandle =
        FindFirstVolumeA(
            nullptr,
            0);

    // FindFirstVolumeA requires a caller-provided buffer, so use the
    // documented maximum volume name size below instead.
    if (findHandle != INVALID_HANDLE_VALUE)
    {
        FindVolumeClose(findHandle);
    }

    char volumeName[MAX_PATH]{};

    HANDLE volumeFindHandle =
        FindFirstVolumeA(
            volumeName,
            ARRAYSIZE(volumeName));

    if (volumeFindHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD errorCode =
            GetLastError();

        if (errorCode == ERROR_NO_MORE_FILES)
        {
            return true;
        }

        result.nativeErrorCode =
            errorCode;

        result.phase =
            "Volume enumeration";

        result.message =
            "Unable to enumerate Windows volumes. Windows error=" +
            std::to_string(errorCode) +
            " (" +
            getWindowsErrorMessage(errorCode) +
            ").";

        return false;
    }

    bool success = true;

    do
    {
        HANDLE volumeHandle =
            CreateFileA(
                volumeName,
                GENERIC_READ |
                    GENERIC_WRITE,
                FILE_SHARE_READ |
                    FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);

        if (volumeHandle == INVALID_HANDLE_VALUE)
        {
            if (!FindNextVolumeA(
                    volumeFindHandle,
                    volumeName,
                    ARRAYSIZE(volumeName)))
            {
                break;
            }

            continue;
        }

        STORAGE_DEVICE_NUMBER volumeDeviceNumber{};

        returnedBytes = 0;

        const BOOL querySuccess =
            DeviceIoControl(
                volumeHandle,
                IOCTL_STORAGE_GET_DEVICE_NUMBER,
                nullptr,
                0,
                &volumeDeviceNumber,
                sizeof(volumeDeviceNumber),
                &returnedBytes,
                nullptr);

        if (querySuccess &&
            volumeDeviceNumber.DeviceType ==
                deviceNumber.DeviceType &&
            volumeDeviceNumber.DeviceNumber ==
                deviceNumber.DeviceNumber)
        {
            std::cout
                << "Target volume detected: "
                << volumeName
                << '\n';

            DWORD bytesReturned = 0;

            const BOOL lockSuccess =
                DeviceIoControl(
                    volumeHandle,
                    FSCTL_LOCK_VOLUME,
                    nullptr,
                    0,
                    nullptr,
                    0,
                    &bytesReturned,
                    nullptr);

            if (!lockSuccess)
            {
                const DWORD errorCode =
                    GetLastError();

                std::cout
                    << "Unable to lock target volume "
                    << volumeName
                    << ". Windows error: "
                    << errorCode
                    << '\n';

                CloseHandle(
                    volumeHandle);

                result.nativeErrorCode =
                    errorCode;

                result.phase =
                    "Volume lock";

                result.message =
                    "Unable to lock target volume before physical overwrite.";

                success = false;
                break;
            }

            const BOOL dismountSuccess =
                DeviceIoControl(
                    volumeHandle,
                    FSCTL_DISMOUNT_VOLUME,
                    nullptr,
                    0,
                    nullptr,
                    0,
                    &bytesReturned,
                    nullptr);

            if (!dismountSuccess)
            {
                const DWORD errorCode =
                    GetLastError();

                std::cout
                    << "Unable to dismount target volume "
                    << volumeName
                    << ". Windows error: "
                    << errorCode
                    << '\n';

                DeviceIoControl(
                    volumeHandle,
                    FSCTL_UNLOCK_VOLUME,
                    nullptr,
                    0,
                    nullptr,
                    0,
                    &bytesReturned,
                    nullptr);

                CloseHandle(
                    volumeHandle);

                result.nativeErrorCode =
                    errorCode;

                result.phase =
                    "Volume dismount";

                result.message =
                    "Unable to dismount target volume before physical overwrite.";

                success = false;
                break;
            }

            std::cout
                << "Volume "
                << volumeName
                << " locked and dismounted successfully.\n";

            lockedVolumes.push_back(
                volumeHandle);
        }
        else
        {
            CloseHandle(
                volumeHandle);
        }

        if (!FindNextVolumeA(
                volumeFindHandle,
                volumeName,
                ARRAYSIZE(volumeName)))
        {
            break;
        }

    } while (true);

    FindVolumeClose(
        volumeFindHandle);

    if (!success)
    {
        unlockTargetVolumes(
            lockedVolumes);

        return false;
    }

    std::cout
        << "\nVolume preparation completed.\n";

    return true;
}

void HostOverwriteSanitizer::unlockTargetVolumes(
    std::vector<HANDLE>& lockedVolumes)
{
    for (HANDLE volumeHandle : lockedVolumes)
    {
        if (volumeHandle == INVALID_HANDLE_VALUE)
            continue;

        DWORD returnedBytes = 0;

        DeviceIoControl(
            volumeHandle,
            FSCTL_UNLOCK_VOLUME,
            nullptr,
            0,
            nullptr,
            0,
            &returnedBytes,
            nullptr);

        CloseHandle(
            volumeHandle);
    }

    lockedVolumes.clear();
}

bool HostOverwriteSanitizer::overwrite(
    HANDLE deviceHandle,
    std::uint64_t totalBytes,
    std::uint32_t sectorSize,
    VerificationResult& result,
    const SecureWipe::SanitizationProgressCallback& progressCallback)
{
    std::size_t chunkSize =
        BUFFER_SIZE;

    chunkSize =
        (chunkSize / sectorSize) *
        sectorSize;

    if (chunkSize == 0)
        chunkSize = sectorSize;

    std::vector<std::uint8_t> buffer(
        chunkSize,
        0x00);

    std::uint64_t offset = 0;

    // The write loop can execute thousands/millions of chunks.
    // Report actual byte progress at a bounded cadence so progress
    // reporting never becomes a significant part of the sanitization
    // workload.
    std::uint64_t lastReportedBytes = 0;

    constexpr std::uint64_t REPORT_INTERVAL =
        64ULL * 1024ULL * 1024ULL; // 64 MiB

    while (offset < totalBytes)
    {
        const std::uint64_t remaining =
            totalBytes - offset;

        DWORD bytesToWrite =
            static_cast<DWORD>(
                std::min<std::uint64_t>(
                    chunkSize,
                    remaining));

        if ((bytesToWrite % sectorSize) != 0)
        {
            if (remaining < sectorSize)
            {
                result.nativeErrorCode =
                    ERROR_INVALID_PARAMETER;

                result.failedOffset =
                    offset;

                result.phase =
                    "Write alignment";

                result.message =
                    "Final write is not sector aligned.";

                return false;
            }

            bytesToWrite =
                (bytesToWrite / sectorSize) *
                sectorSize;
        }

        LARGE_INTEGER position{};

        position.QuadPart =
            static_cast<LONGLONG>(
                offset);

        if (!SetFilePointerEx(
                deviceHandle,
                position,
                nullptr,
                FILE_BEGIN))
        {
            const DWORD errorCode =
                GetLastError();

            result.nativeErrorCode =
                errorCode;

            result.failedOffset =
                offset;

            result.requestedBytes =
                bytesToWrite;

            result.actualBytes =
                0;

            result.phase =
                "Write seek";

            result.message =
                buildIoError(
                    "Write seek",
                    errorCode,
                    offset,
                    bytesToWrite,
                    0);

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
            const DWORD errorCode =
                GetLastError();

            result.nativeErrorCode =
                errorCode;

            result.failedOffset =
                offset;

            result.requestedBytes =
                bytesToWrite;

            result.actualBytes =
                bytesWritten;

            result.phase =
                "WriteFile";

            result.message =
                buildIoError(
                    "WriteFile",
                    errorCode,
                    offset,
                    bytesToWrite,
                    bytesWritten);

            return false;
        }

        if (bytesWritten != bytesToWrite)
        {
            result.nativeErrorCode =
                ERROR_WRITE_FAULT;

            result.failedOffset =
                offset;

            result.requestedBytes =
                bytesToWrite;

            result.actualBytes =
                bytesWritten;

            result.phase =
                "Partial write";

            result.message =
                buildIoError(
                    "Partial write",
                    ERROR_WRITE_FAULT,
                    offset,
                    bytesToWrite,
                    bytesWritten);

            return false;
        }

        offset +=
            bytesWritten;

        const int progress =
            static_cast<int>(
                (offset * 100ULL) /
                totalBytes);

        std::cout
            << "\rHost overwrite: "
            << progress
            << "%"
            << std::flush;

        if (
            progressCallback &&
            (
                offset - lastReportedBytes >= REPORT_INTERVAL ||
                offset >= totalBytes
            ))
        {
            lastReportedBytes =
                offset;

            progressCallback(
                offset,
                totalBytes,
                "HOST_OVERWRITE",
                "Writing overwrite pattern");
        }
    }

    std::cout
        << "\nHost overwrite completed.\n";

    if (progressCallback)
    {
        progressCallback(
            totalBytes,
            totalBytes,
            "HOST_OVERWRITE",
            "Host overwrite completed");
    }

    result.deviceReportedSuccess =
        true;

    return true;
}

VerificationResult HostOverwriteSanitizer::verify(
    HANDLE deviceHandle,
    std::uint64_t totalBytes,
    std::uint32_t sectorSize)
{
    VerificationResult result;

    result.performed =
        true;

    result.method =
        VerificationMethod::HOST_READ_BACK;

    if (totalBytes < VERIFY_SIZE)
    {
        result.performed =
            false;

        result.phase =
            "Verification preparation";

        result.message =
            "Target is smaller than verification block.";

        return result;
    }

    std::vector<std::uint8_t> buffer(
        VERIFY_SIZE);

    const std::uint64_t offsets[] =
    {
        0,
        totalBytes / 4,
        totalBytes / 2,
        (totalBytes * 3) / 4,
        totalBytes - VERIFY_SIZE
    };

    for (std::uint64_t offset : offsets)
    {
        offset =
            (offset / sectorSize) *
            sectorSize;

        if (offset + VERIFY_SIZE > totalBytes)
        {
            offset =
                ((totalBytes - VERIFY_SIZE) /
                 sectorSize) *
                sectorSize;
        }

        LARGE_INTEGER position{};

        position.QuadPart =
            static_cast<LONGLONG>(
                offset);

        if (!SetFilePointerEx(
                deviceHandle,
                position,
                nullptr,
                FILE_BEGIN))
        {
            const DWORD errorCode =
                GetLastError();

            result.nativeErrorCode =
                errorCode;

            result.failedOffset =
                offset;

            result.requestedBytes =
                static_cast<std::uint32_t>(
                    VERIFY_SIZE);

            result.actualBytes =
                0;

            result.phase =
                "Verification seek";

            result.message =
                buildIoError(
                    "Verification seek",
                    errorCode,
                    offset,
                    static_cast<DWORD>(
                        VERIFY_SIZE),
                    0);

            return result;
        }

        DWORD bytesRead = 0;

        if (!ReadFile(
                deviceHandle,
                buffer.data(),
                static_cast<DWORD>(
                    VERIFY_SIZE),
                &bytesRead,
                nullptr))
        {
            const DWORD errorCode =
                GetLastError();

            result.nativeErrorCode =
                errorCode;

            result.failedOffset =
                offset;

            result.requestedBytes =
                static_cast<std::uint32_t>(
                    VERIFY_SIZE);

            result.actualBytes =
                bytesRead;

            result.phase =
                "Verification read";

            result.message =
                buildIoError(
                    "Verification read",
                    errorCode,
                    offset,
                    static_cast<DWORD>(
                        VERIFY_SIZE),
                    bytesRead);

            return result;
        }

        if (bytesRead != VERIFY_SIZE)
        {
            result.nativeErrorCode =
                ERROR_READ_FAULT;

            result.failedOffset =
                offset;

            result.requestedBytes =
                static_cast<std::uint32_t>(
                    VERIFY_SIZE);

            result.actualBytes =
                bytesRead;

            result.phase =
                "Verification read";

            result.message =
                buildIoError(
                    "Incomplete verification read",
                    ERROR_READ_FAULT,
                    offset,
                    static_cast<DWORD>(
                        VERIFY_SIZE),
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

        result.bytesVerified +=
            bytesRead;

        result.samples++;

        if (!allZero)
        {
            result.failedOffset =
                offset;

            result.phase =
                "Verification data check";

            result.message =
                "Verification failed: non-zero data detected at offset " +
                std::to_string(offset) +
                ".";

            return result;
        }

        std::cout
            << "Verification sample "
            << result.samples
            << " PASSED at offset "
            << offset
            << ".\n";
    }

    result.passed =
        true;

    result.sanitizationCompleted =
        true;

    result.message =
        "Host overwrite completed and read-back verification passed.";

    std::cout
        << "\n"
        << result.message
        << '\n';

    return result;
}

VerificationResult HostOverwriteSanitizer::sanitize(
    HANDLE deviceHandle,
    std::uint64_t totalBytes,
    const SecureWipe::SanitizationProgressCallback& progressCallback)
{
    VerificationResult result;

    result.method =
        VerificationMethod::HOST_READ_BACK;

    if (progressCallback)
    {
        progressCallback(
            0,
            totalBytes,
            "HOST_OVERWRITE",
            "Preparing host overwrite");
    }

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.phase =
            "Handle validation";

        result.message =
            "Sanitization failed: invalid device handle.";

        return result;
    }

    if (totalBytes == 0)
    {
        result.phase =
            "Capacity validation";

        result.message =
            "Sanitization failed: device capacity is zero.";

        return result;
    }

    std::uint32_t sectorSize = 0;

    if (!getSectorSize(
            deviceHandle,
            sectorSize))
    {
        const DWORD errorCode =
            GetLastError();

        result.nativeErrorCode =
            errorCode;

        result.phase =
            "Sector size detection";

        result.message =
            "Unable to determine device sector size. Windows error=" +
            std::to_string(errorCode) +
            " (" +
            getWindowsErrorMessage(errorCode) +
            ").";

        return result;
    }

    std::cout
        << "\nPhysical sector size: "
        << sectorSize
        << " bytes\n";

    if ((totalBytes % sectorSize) != 0)
    {
        result.nativeErrorCode =
            ERROR_INVALID_DATA;

        result.phase =
            "Capacity alignment";

        result.message =
            "Device capacity is not aligned to reported sector size.";

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

    std::vector<HANDLE> lockedVolumes;

    if (!lockTargetVolumes(
            deviceHandle,
            lockedVolumes,
            result))
    {
        return result;
    }

    std::cout
        << "\n========================================\n"
        << " RAW PHYSICAL DEVICE OVERWRITE\n"
        << "========================================\n";

    if (!overwrite(
            deviceHandle,
            totalBytes,
            sectorSize,
            result,
            progressCallback))
    {
        unlockTargetVolumes(
            lockedVolumes);

        return result;
    }

    std::cout
        << "Flushing device buffers...\n";

    if (!FlushFileBuffers(
            deviceHandle))
    {
        const DWORD errorCode =
            GetLastError();

        result.nativeErrorCode =
            errorCode;

        result.phase =
            "FlushFileBuffers";

        result.message =
            "FlushFileBuffers failed. Windows error=" +
            std::to_string(errorCode) +
            " (" +
            getWindowsErrorMessage(errorCode) +
            ").";

        unlockTargetVolumes(
            lockedVolumes);

        return result;
    }

    std::cout
        << "Device buffers flushed successfully.\n";

    std::cout
        << "\nStarting sampled read-back verification...\n";

    VerificationResult verificationResult =
        verify(
            deviceHandle,
            totalBytes,
            sectorSize);

    unlockTargetVolumes(
        lockedVolumes);

    if (!verificationResult.passed)
    {
        result =
            verificationResult;

        return result;
    }

    result =
        verificationResult;

    return result;
}