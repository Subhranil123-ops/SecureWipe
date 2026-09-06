#include "WindowsStorageUtils.h"
#include <iostream>

bool WindowsStorageUtils::getWindowsDirectory(std::wstring &windowsDirectory)
{
    wchar_t buffer[MAX_PATH] = {};

    UINT length = GetWindowsDirectoryW(
        buffer,
        MAX_PATH);

    if (length == 0)
    {
        return false;
    }

    windowsDirectory = buffer;

    return true;
};

bool WindowsStorageUtils::getWindowsDrive(const std::wstring &windowsDirectory, std::wstring &windowsDrive)
{
    if (windowsDirectory.size() < 2)
    {
        return false;
    }

    if (windowsDirectory[1] != L':')
    {
        return false;
    }

    windowsDrive =
        windowsDirectory.substr(0, 2);

    return true;
};

bool WindowsStorageUtils::getVolumeGuid(std::wstring &windowsDrive, std::wstring &volumeGuid)
{
    std::wstring mountPoint = windowsDrive + L"\\";

    wchar_t buffer[MAX_PATH] = {};

    BOOL result = GetVolumeNameForVolumeMountPointW(mountPoint.c_str(), buffer, MAX_PATH);

    if (!result)
    {
        std::cout << "Failed to get volume GUID\n";
        return false;
    }

    volumeGuid = buffer;

    return true;
};

bool WindowsStorageUtils::getPhysicalDisk(
    const std::wstring &drive,
    DWORD &diskNumber,
    DWORD &partitionNumber)
{
    std::wstring devicePath = L"\\\\.\\" + drive;

    HANDLE handle = CreateFileW(
        devicePath.c_str(),                 // \\.\C:
        0,                                  // special read/write access nahi
        FILE_SHARE_READ | FILE_SHARE_WRITE, // sharing allow
        nullptr,                            // default security
        OPEN_EXISTING,                      // existing device open karo
        0,                                  // default flags
        nullptr                             // template nahi
    );

    if (handle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Failed to open drive\n";
        return false;
    }

    STORAGE_DEVICE_NUMBER deviceNumber = {};
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        handle,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        nullptr,
        0,
        &deviceNumber,
        sizeof(deviceNumber),
        &bytesReturned,
        nullptr);

    CloseHandle(handle);

    if (!result)
    {
        std::cout << "Failed to get physical disk\n";
        return false;
    }

    diskNumber = deviceNumber.DeviceNumber;
    partitionNumber = deviceNumber.PartitionNumber;

    return true;
};

bool WindowsStorageUtils::getVolumePathFromFile(const std::wstring &filePath, std::wstring &volumePath)
{
    if (filePath.empty())
    {
        return false;
    }

    wchar_t buffer[MAX_PATH] = {};

    BOOL result = GetVolumePathNameW(
        filePath.c_str(),
        buffer,
        MAX_PATH);

    if (!result)
    {
        return false;
    }

    volumePath = buffer;
    return true;
};

bool WindowsStorageUtils::getPhysicalDiskFromFile(const std::wstring &filePath, DWORD &diskNumber, DWORD &partitionNumber)
{
    std::wstring volumePath;

    if (!getVolumePathFromFile(filePath, volumePath))
    {
        return false;
    }

    if (volumePath.size() < 2)
        return false;

    std::wstring drive = volumePath.substr(0, 2);

    return getPhysicalDisk(drive, diskNumber, partitionNumber);
}

bool WindowsStorageUtils::getDiskNumberFromDeviceId(const std::string &deviceId, DWORD &diskNumber)
{
    const std::string prefix =
        "\\\\.\\PhysicalDrive";

    if (deviceId.rfind(prefix, 0) != 0)
    {
        return false;
    }

    std::string numberText =
        deviceId.substr(prefix.size());

    if (numberText.empty())
    {
        return false;
    }

    try
    {
        diskNumber =
            static_cast<DWORD>(
                std::stoul(numberText));
    }
    catch (...)
    {
        return false;
    }

    return true;
}