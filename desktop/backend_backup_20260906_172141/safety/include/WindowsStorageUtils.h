#pragma once
#include <string>
#include <Windows.h>

namespace WindowsStorageUtils
{
    // Get the Windows installation directory.
    // Example:
    // C:\Windows
    bool getWindowsDirectory(std::wstring &windowsDirectory);

    // Get the drive containing the Windows directory.
    // Example:
    // C:\Windows -> C:
    bool getWindowsDrive(const std::wstring &windowsDirectory, std::wstring &windowsDrive);

    // Get the Volume GUID of a drive.
    // Example:
    //  C: -> \\?\Volume{GUID}

    bool getVolumeGuid(std::wstring &windowsDrive, std::wstring &volumeGuid);

    // Get physical disk and partition information
    // for a drive.
    //
    // Example:
    // C:
    //  -> PhysicalDrive0
    //  -> Partition 3
    bool getPhysicalDisk(const std::wstring &drive, DWORD &diskNumber, DWORD &partitionNumber);

    bool getVolumePathFromFile(const std::wstring &filePath, std::wstring &volumePath);

    bool getPhysicalDiskFromFile(const std::wstring &filePath, DWORD &diskNumber, DWORD &partitionNumber);

    bool getDiskNumberFromDeviceId(const std::string &deviceId, DWORD &diskNumber);

};