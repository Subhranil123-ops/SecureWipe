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

    stream << phase
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

bool getPhysicalDiskNumber(
    HANDLE deviceHandle,
    DWORD& diskNumber)
{
    STORAGE_DEVICE_NUMBER deviceNumber{};
    DWORD returnedBytes = 0;

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
        return false;
    }

    diskNumber = deviceNumber.DeviceNumber;
    return true;
}

bool getVolumeDiskNumber(
    HANDLE volumeHandle,
    DWORD& diskNumber)
{
    STORAGE_DEVICE_NUMBER deviceNumber{};
    DWORD returnedBytes = 0;

    if (!DeviceIoControl(
            volumeHandle,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            nullptr,
            0,
            &deviceNumber,
            sizeof(deviceNumber),
            &returnedBytes,
            nullptr))
    {
        return false;
    }

    diskNumber = deviceNumber.DeviceNumber;
    return true;
}
}

bool HostOverwriteSanitizer::getSectorSize(
    HANDLE deviceHandle,
    std::uint32_t& sectorSize)
{
    DISK_GEOMETRY_EX geometry{};
    DWORD returnedBytes = 0;

    if (!DeviceIoControl(
            deviceHandle,
            IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            nullptr,
            0,
            &geometry,
            sizeof(geometry),
            &returnedBytes,
            nullptr))
    {
        return false;
    }

    if (geometry.Geometry.BytesPerSector == 0)
        return false;

    sectorSize =
        static_cast<std::uint32_t>(
            geometry.Geometry.BytesPerSector);

    return true;
}

bool HostOverwriteSanitizer::checkWritable(
    HANDLE deviceHandle,
    VerificationResult& result)
{
    DWORD returnedBytes = 0;

    if (DeviceIoControl(
            deviceHandle,
            IOCTL_DISK_IS_WRITABLE,
            nullptr,
            0,
            nullptr,
            0,
            &returnedBytes,
            nullptr))
    {
        return true;
    }

    const DWORD errorCode = GetLastError();

    if (errorCode == ERROR_INVALID_FUNCTION ||
        errorCode == ERROR_NOT_SUPPORTED)
    {
        std::cout
            << "Writable preflight is not supported by this device.\n"
            << "Continuing to actual write operation.\n";

        return true;
    }

    result.nativeErrorCode = errorCode;
    result.phase = "Writable preflight";
    result.message =
        "Target device is not writable. Windows error=" +
        std::to_string(errorCode) +
        " (" +
        getWindowsErrorMessage(errorCode) +
        ").";

    return false;
}

bool HostOverwriteSanitizer::lockTargetVolumes(
    HANDLE deviceHandle,
    std::vector<HANDLE>& lockedVolumes,
    VerificationResult& result)
{
    DWORD targetDiskNumber = 0;

    if (!getPhysicalDiskNumber(
            deviceHandle,
            targetDiskNumber))
    {
        const DWORD errorCode = GetLastError();

        result.nativeErrorCode = errorCode;
        result.phase = "Physical disk identification";
        result.message =
            "Unable to determine target physical disk number. Windows error=" +
            std::to_string(errorCode) +
            " (" +
            getWindowsErrorMessage(errorCode) +
            ").";

        return false;
    }

    std::cout
        << "\nTarget PhysicalDrive number: "
        << targetDiskNumber
        << '\n';

    const DWORD logicalDrives = GetLogicalDrives();

    if (logicalDrives == 0)
    {
        const DWORD errorCode = GetLastError();

        result.nativeErrorCode = errorCode;
        result.phase = "Logical drive enumeration";
        result.message =
            "Unable to enumerate logical drives. Windows error=" +
            std::to_string(errorCode) +
            " (" +
            getWindowsErrorMessage(errorCode) +
            ").";

        return false;
    }

    std::cout
        << "Checking mounted volumes belonging to target disk...\n";

    for (char driveLetter = 'A';
         driveLetter <= 'Z';
         ++driveLetter)
    {
        const DWORD mask =
            1u << (driveLetter - 'A');

        if ((logicalDrives & mask) == 0)
            continue;

        std::string volumePath = "\\\\.\\";
        volumePath += driveLetter;
        volumePath += ":";

        HANDLE volumeHandle =
            CreateFileA(
                volumePath.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);

        if (volumeHandle == INVALID_HANDLE_VALUE)
        {
            const DWORD errorCode = GetLastError();

            std::cout
                << "\nUnable to open logical volume "
                << driveLetter
                << ": for lock preparation.\n"
                << "Windows error: "
                << errorCode
                << " ("
                << getWindowsErrorMessage(errorCode)
                << ")\n";

            /*
             * We cannot determine whether this volume belongs to the
             * target disk because the volume handle could not be opened.
             *
             * For a destructive operation we fail closed rather than
             * silently continuing.
             */
            result.nativeErrorCode = errorCode;
            result.phase =
                "Target volume handle open";

            result.message =
                "Unable to open logical volume " +
                std::string(1, driveLetter) +
                ": before destructive overwrite. Windows error=" +
                std::to_string(errorCode) +
                " (" +
                getWindowsErrorMessage(errorCode) +
                "). Close applications using mounted volumes and retry.";

            unlockTargetVolumes(lockedVolumes);
            return false;
        }

        DWORD volumeDiskNumber = 0;

        if (!getVolumeDiskNumber(
                volumeHandle,
                volumeDiskNumber))
        {
            const DWORD errorCode = GetLastError();

            std::cout
                << "\nUnable to determine the physical disk for volume "
                << driveLetter
                << ":.\n"
                << "Windows error: "
                << errorCode
                << " ("
                << getWindowsErrorMessage(errorCode)
                << ")\n";

            result.nativeErrorCode = errorCode;
            result.phase =
                "Volume physical-disk identification";

            result.message =
                "Unable to determine which physical disk owns volume " +
                std::string(1, driveLetter) +
                ":. Destructive operation aborted.";

            CloseHandle(volumeHandle);
            unlockTargetVolumes(lockedVolumes);
            return false;
        }

        if (volumeDiskNumber != targetDiskNumber)
        {
            CloseHandle(volumeHandle);
            continue;
        }

        std::cout
            << "\nTarget volume found: "
            << driveLetter
            << ":\n";

        DWORD returnedBytes = 0;

        std::cout
            << "Locking volume "
            << driveLetter
            << ": ...\n";

        if (!DeviceIoControl(
                volumeHandle,
                FSCTL_LOCK_VOLUME,
                nullptr,
                0,
                nullptr,
                0,
                &returnedBytes,
                nullptr))
        {
            const DWORD errorCode = GetLastError();

            result.nativeErrorCode = errorCode;
            result.phase = "FSCTL_LOCK_VOLUME";

            result.message =
                "Unable to lock target volume " +
                std::string(1, driveLetter) +
                ":. Windows error=" +
                std::to_string(errorCode) +
                " (" +
                getWindowsErrorMessage(errorCode) +
                "). Close Explorer or applications using the target drive.";

            CloseHandle(volumeHandle);
            unlockTargetVolumes(lockedVolumes);

            return false;
        }

        std::cout
            << "Volume "
            << driveLetter
            << ": locked successfully.\n";

        returnedBytes = 0;

        std::cout
            << "Dismounting volume "
            << driveLetter
            << ": ...\n";

        if (!DeviceIoControl(
                volumeHandle,
                FSCTL_DISMOUNT_VOLUME,
                nullptr,
                0,
                nullptr,
                0,
                &returnedBytes,
                nullptr))
        {
            const DWORD errorCode = GetLastError();

            result.nativeErrorCode = errorCode;
            result.phase = "FSCTL_DISMOUNT_VOLUME";

            result.message =
                "Unable to dismount target volume " +
                std::string(1, driveLetter) +
                ":. Windows error=" +
                std::to_string(errorCode) +
                " (" +
                getWindowsErrorMessage(errorCode) +
                ").";

            returnedBytes = 0;

            DeviceIoControl(
                volumeHandle,
                FSCTL_UNLOCK_VOLUME,
                nullptr,
                0,
                nullptr,
                0,
                &returnedBytes,
                nullptr);

            CloseHandle(volumeHandle);
            unlockTargetVolumes(lockedVolumes);

            return false;
        }

        std::cout
            << "Volume "
            << driveLetter
            << ": dismounted successfully.\n";

        lockedVolumes.push_back(volumeHandle);
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

        CloseHandle(volumeHandle);
    }

    lockedVolumes.clear();
}

bool HostOverwriteSanitizer::overwrite(
    HANDLE deviceHandle,
    std::uint64_t totalBytes,
    std::uint32_t sectorSize,
    VerificationResult& result)
{
    std::size_t chunkSize = BUFFER_SIZE;

    chunkSize =
        (chunkSize / sectorSize) *
        sectorSize;

    if (chunkSize == 0)
        chunkSize = sectorSize;

    std::vector<std::uint8_t> buffer(
        chunkSize,
        0x00);

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
            result.requestedBytes = bytesToWrite;
            result.actualBytes = 0;
            result.phase = "Write seek";

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
            << "%"
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

    result.performed = true;
    result.method =
        VerificationMethod::HOST_READ_BACK;

    if (totalBytes < VERIFY_SIZE)
    {
        result.performed = false;
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
            result.requestedBytes =
                static_cast<std::uint32_t>(VERIFY_SIZE);
            result.actualBytes = 0;
            result.phase =
                "Verification seek";

            result.message =
                buildIoError(
                    "Verification seek",
                    errorCode,
                    offset,
                    static_cast<DWORD>(VERIFY_SIZE),
                    0);

            return result;
        }

        DWORD bytesRead = 0;

        if (!ReadFile(
                deviceHandle,
                buffer.data(),
                static_cast<DWORD>(VERIFY_SIZE),
                &bytesRead,
                nullptr))
        {
            const DWORD errorCode = GetLastError();

            result.nativeErrorCode = errorCode;
            result.failedOffset = offset;
            result.requestedBytes =
                static_cast<std::uint32_t>(VERIFY_SIZE);
            result.actualBytes = bytesRead;
            result.phase =
                "Verification read";

            result.message =
                buildIoError(
                    "Verification read",
                    errorCode,
                    offset,
                    static_cast<DWORD>(VERIFY_SIZE),
                    bytesRead);

            return result;
        }

        if (bytesRead != VERIFY_SIZE)
        {
            result.nativeErrorCode =
                ERROR_READ_FAULT;

            result.failedOffset = offset;
            result.requestedBytes =
                static_cast<std::uint32_t>(VERIFY_SIZE);
            result.actualBytes = bytesRead;
            result.phase =
                "Verification read";

            result.message =
                buildIoError(
                    "Incomplete verification read",
                    ERROR_READ_FAULT,
                    offset,
                    static_cast<DWORD>(VERIFY_SIZE),
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

        result.bytesVerified += bytesRead;
        result.samples++;

        if (!allZero)
        {
            result.failedOffset = offset;
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

    result.passed = true;
    result.sanitizationCompleted = true;

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
    std::uint64_t totalBytes)
{
    VerificationResult result;

    result.method =
        VerificationMethod::HOST_READ_BACK;

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
        const DWORD errorCode = GetLastError();

        result.nativeErrorCode = errorCode;
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
            result))
    {
        unlockTargetVolumes(
            lockedVolumes);

        return result;
    }

    std::cout
        << "Flushing device buffers...\n";

    if (!FlushFileBuffers(deviceHandle))
    {
        const DWORD errorCode = GetLastError();

        result.nativeErrorCode = errorCode;
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

    return verificationResult;
}