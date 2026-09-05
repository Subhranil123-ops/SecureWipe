#include "HostOverwriteSanitizer.h"

#include "WindowsStorageDiscovery.h"
#include "StorageDevice.h"

#include <Windows.h>
#include <ntdddisk.h>

#include <cstdint>
#include <iostream>
#include <string>

namespace
{
void printDeviceInfo(const StorageDevice& device)
{
    std::cout << "\nDevice Information\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Device ID    : " << device.getDeviceId() << '\n';
    std::cout << "Model        : " << device.getModel() << '\n';
    std::cout << "Serial       : " << device.getSerialNumber() << '\n';
    std::cout << "Interface    : " << device.getInterfaceType() << '\n';
    std::cout << "Capacity     : " << device.getCapacityBytes() << " bytes\n";
    std::cout << "System Disk  : " << (device.isSystemDisk() ? "YES" : "NO") << '\n';
    std::cout << "Removable    : " << (device.isRemovable() ? "YES" : "NO") << '\n';
}

bool openDevice(const std::string& devicePath, HANDLE& deviceHandle)
{
    deviceHandle = CreateFileA(
        devicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "\n[FAIL] Failed to open target device.\n";
        std::cout << "Windows Error : " << GetLastError() << '\n';
        return false;
    }

    return true;
}

bool getDeviceCapacity(HANDLE deviceHandle, std::uint64_t& capacity)
{
    GET_LENGTH_INFORMATION lengthInfo{};
    DWORD returnedBytes = 0;

    if (!DeviceIoControl(
            deviceHandle,
            IOCTL_DISK_GET_LENGTH_INFO,
            nullptr,
            0,
            &lengthInfo,
            sizeof(lengthInfo),
            &returnedBytes,
            nullptr))
    {
        std::cout << "\n[FAIL] Failed to determine device capacity.\n";
        std::cout << "Windows Error : " << GetLastError() << '\n';
        return false;
    }

    if (lengthInfo.Length.QuadPart <= 0)
    {
        std::cout << "\n[FAIL] Invalid device capacity.\n";
        return false;
    }

    capacity = static_cast<std::uint64_t>(lengthInfo.Length.QuadPart);
    return true;
}

bool confirmDestructiveOperation(const StorageDevice& device)
{
    std::cout << "\n============================================================\n";
    std::cout << "                 DESTRUCTIVE OPERATION\n";
    std::cout << "============================================================\n";

    std::cout << "\nWARNING: This operation will permanently destroy data.\n";
    std::cout << "Target device:\n";

    printDeviceInfo(device);

    std::cout << "\nType EXACTLY:\n";
    std::cout << "ERASE " << device.getSerialNumber() << '\n';
    std::cout << "to continue: ";

    std::string confirmation;
    std::getline(std::cin >> std::ws, confirmation);

    const std::string expected = "ERASE " + device.getSerialNumber();

    if (confirmation != expected)
    {
        std::cout << "\n[ABORTED] Confirmation did not match.\n";
        return false;
    }

    return true;
}
}

int main()
{
    std::cout << "\n============================================================\n";
    std::cout << "             HOST OVERWRITE SANITIZER TEST\n";
    std::cout << "============================================================\n";

    std::cout << "\n[1] Discovering storage devices...\n";

    WindowsStorageDiscovery discovery;
    const auto devices = discovery.discover();

    if (devices.empty())
    {
        std::cout << "\n[FAIL] No storage devices discovered.\n";
        return 1;
    }

    std::cout << "\nDiscovered Devices\n";
    std::cout << "------------------------------------------------------------\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
    {
        const auto& device = devices[i];

        std::cout << "\n[" << i << "]\n";
        std::cout << "Device ID   : " << device.getDeviceId() << '\n';
        std::cout << "Model       : " << device.getModel() << '\n';
        std::cout << "Serial      : " << device.getSerialNumber() << '\n';
        std::cout << "Interface   : " << device.getInterfaceType() << '\n';
        std::cout << "Capacity    : " << device.getCapacityBytes() << " bytes\n";
        std::cout << "System Disk : " << (device.isSystemDisk() ? "YES" : "NO") << '\n';
        std::cout << "Removable   : " << (device.isRemovable() ? "YES" : "NO") << '\n';
    }

    std::cout << "\nEnter device index to test: ";

    std::size_t deviceIndex = 0;
    std::cin >> deviceIndex;

    if (deviceIndex >= devices.size())
    {
        std::cout << "\n[FAIL] Invalid device selection.\n";
        return 1;
    }

    const StorageDevice& target = devices[deviceIndex];

    std::cout << "\n[2] Validating target...\n";

    if (target.isSystemDisk())
    {
        std::cout << "\n[BLOCKED] Selected device is the system disk.\n";
        std::cout << "Destructive testing is not allowed.\n";
        return 1;
    }

    if (target.getDeviceId().empty())
    {
        std::cout << "\n[FAIL] Device ID is missing.\n";
        return 1;
    }

    if (target.getCapacityBytes() == 0)
    {
        std::cout << "\n[FAIL] Device capacity is unknown.\n";
        return 1;
    }

    if (target.isRemovable())
        std::cout << "Target type : Removable device\n";
    else
        std::cout << "Target type : Internal device\n";

    printDeviceInfo(target);

    if (!confirmDestructiveOperation(target))
        return 0;

    HANDLE deviceHandle = INVALID_HANDLE_VALUE;

    std::cout << "\n[3] Opening target device...\n";

    if (!openDevice(target.getDeviceId(), deviceHandle))
        return 1;

    std::cout << "Device opened successfully.\n";

    std::uint64_t actualCapacity = 0;

    std::cout << "\n[4] Reading physical device capacity...\n";

    if (!getDeviceCapacity(deviceHandle, actualCapacity))
    {
        CloseHandle(deviceHandle);
        return 1;
    }

    std::cout << "Physical Capacity : " << actualCapacity << " bytes\n";

    if (actualCapacity != target.getCapacityBytes())
    {
        std::cout << "\n[WARNING] Discovered capacity and physical capacity "
                     "do not match.\n";

        std::cout << "Discovered : " << target.getCapacityBytes() << '\n';
        std::cout << "Physical   : " << actualCapacity << '\n';

        std::cout << "\n[ABORTED] Target identity/capacity mismatch.\n";

        CloseHandle(deviceHandle);
        return 1;
    }

    std::cout << "\n[5] Starting host overwrite...\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Target       : " << target.getDeviceId() << '\n';
    std::cout << "Interface    : " << target.getInterfaceType() << '\n';
    std::cout << "Operation    : Full-device host overwrite\n";
    std::cout << "Pattern      : 0x00\n";
    std::cout << "Verification : Sampled post-write read-back verification\n";

    HostOverwriteSanitizer sanitizer;

    const VerificationResult result =
        sanitizer.sanitize(deviceHandle, actualCapacity);

    CloseHandle(deviceHandle);

    std::cout << "\n------------------------------------------------------------\n";

    std::cout << "Host Overwrite Result\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Verification Performed : "
              << (result.performed ? "YES" : "NO") << '\n';
    std::cout << "Verification Result   : "
              << (result.passed ? "PASSED" : "FAILED") << '\n';
    std::cout << "Verification Samples  : "
              << result.samples << '\n';
    std::cout << "Bytes Verified        : "
              << result.bytesVerified << '\n';
    std::cout << "Message               : "
              << result.message << '\n';

    std::cout << "============================================================\n";

    if (!result.performed)
    {
        std::cout << "FINAL RESULT : FAIL - Verification was not performed.\n";
        return 1;
    }

    if (!result.passed)
    {
        std::cout << "FINAL RESULT : FAIL - Verification failed.\n";
        return 1;
    }

    std::cout << "FINAL RESULT : PASS\n";
    return 0;
}