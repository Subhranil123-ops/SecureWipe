#include "WindowsBootChecker.h"

#include <Windows.h>
#include <Wbemidl.h>
#include <winioctl.h>

#include <string>
#include <iostream>

#pragma comment(lib, "wbemuuid.lib")


// ------------------------------------------------------------
// Convert wide string to normal string
// ------------------------------------------------------------

std::string wideToString(std::wstring value)
{
    if (value.empty())
    {
        return "";
    }

    int size =
        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.c_str(),
            -1,
            nullptr,
            0,
            nullptr,
            nullptr);

    if (size <= 0)
    {
        return "";
    }

    char* buffer =
        new char[size];

    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        -1,
        buffer,
        size,
        nullptr,
        nullptr);

    std::string result =
        buffer;

    delete[] buffer;

    return result;
}


// ------------------------------------------------------------
// Get Windows directory
//
// Example:
// C:\Windows
// ------------------------------------------------------------

bool getWindowsDirectory(
    std::wstring& windowsDirectory)
{
    wchar_t buffer[MAX_PATH] = {};

    UINT length =
        GetWindowsDirectoryW(
            buffer,
            MAX_PATH);

    if (length == 0 ||
        length >= MAX_PATH)
    {
        return false;
    }

    windowsDirectory =
        buffer;

    return true;
}


// ------------------------------------------------------------
// Get Windows drive
//
// C:\Windows
//     ↓
// C:
// ------------------------------------------------------------

bool getWindowsDrive(
    std::wstring windowsDirectory,
    std::wstring& windowsDrive)
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
}


// ------------------------------------------------------------
// Get volume GUID
//
// C:
// ↓
// \\?\Volume{GUID}\
// ------------------------------------------------------------

bool getVolumeGuid(
    std::wstring drive,
    std::wstring& volumeGuid)
{
    std::wstring mountPoint =
        drive + L"\\";

    wchar_t buffer[MAX_PATH] = {};

    BOOL result =
        GetVolumeNameForVolumeMountPointW(
            mountPoint.c_str(),
            buffer,
            MAX_PATH);

    if (!result)
    {
        return false;
    }

    volumeGuid =
        buffer;

    return true;
}


// ------------------------------------------------------------
// Get physical disk from a volume
//
// C:
// ↓
// \\.\C:
// ↓
// IOCTL_STORAGE_GET_DEVICE_NUMBER
// ↓
// PhysicalDrive0
// ------------------------------------------------------------

bool getPhysicalDisk(
    std::wstring drive,
    DWORD& diskNumber,
    DWORD& partitionNumber)
{
    std::wstring devicePath =
        L"\\\\.\\" + drive;

    HANDLE handle =
        CreateFileW(
            devicePath.c_str(),
            0,
            FILE_SHARE_READ |
                FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

    if (handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    STORAGE_DEVICE_NUMBER deviceNumber = {};

    DWORD bytesReturned = 0;

    BOOL result =
        DeviceIoControl(
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
        return false;
    }

    diskNumber =
        deviceNumber.DeviceNumber;

    partitionNumber =
        deviceNumber.PartitionNumber;

    return true;
}


// ------------------------------------------------------------
// Initialize WMI
// ------------------------------------------------------------

bool initializeWmi(
    IWbemLocator*& locator,
    IWbemServices*& services)
{
    locator = nullptr;
    services = nullptr;

    HRESULT result =
        CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

    if (FAILED(result) &&
        result != RPC_E_CHANGED_MODE)
    {
        return false;
    }

    result =
        CoInitializeSecurity(
            nullptr,
            -1,
            nullptr,
            nullptr,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE,
            nullptr);

    if (FAILED(result) &&
        result != RPC_E_TOO_LATE)
    {
        CoUninitialize();

        return false;
    }

    result =
        CoCreateInstance(
            CLSID_WbemLocator,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            reinterpret_cast<void**>(
                &locator));

    if (FAILED(result))
    {
        CoUninitialize();

        return false;
    }

    BSTR namespaceName =
        SysAllocString(
            L"ROOT\\WMI");

    if (namespaceName == nullptr)
    {
        locator->Release();

        CoUninitialize();

        return false;
    }

    result =
        locator->ConnectServer(
            namespaceName,
            nullptr,
            nullptr,
            nullptr,
            0,
            nullptr,
            nullptr,
            &services);

    SysFreeString(
        namespaceName);

    if (FAILED(result))
    {
        locator->Release();

        CoUninitialize();

        return false;
    }

    result =
        CoSetProxyBlanket(
            services,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            nullptr,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE);

    if (FAILED(result))
    {
        services->Release();
        locator->Release();

        CoUninitialize();

        return false;
    }

    return true;
}


// ------------------------------------------------------------
// Get BCD system partition
//
// System BCD store:
//
// BcdStore.FilePath=""
//
// Then:
//
// GetSystemPartition()
//
// returns a device path such as:
//
// \Device\HarddiskVolume2
// ------------------------------------------------------------

bool getSystemPartitionFromBcd(
    IWbemServices* services,
    std::wstring& partitionPath)
{
    partitionPath.clear();

    if (services == nullptr)
    {
        return false;
    }

    /*
     * Open the system BCD store directly.
     *
     * We no longer depend on reading __PATH from
     * an object returned by OpenStore().
     */

    BSTR objectPath =
        SysAllocString(
            L"BcdStore.FilePath=\"\"");

    BSTR methodName =
        SysAllocString(
            L"GetSystemPartition");

    if (objectPath == nullptr ||
        methodName == nullptr)
    {
        if (objectPath != nullptr)
        {
            SysFreeString(
                objectPath);
        }

        if (methodName != nullptr)
        {
            SysFreeString(
                methodName);
        }

        return false;
    }

    IWbemClassObject* output =
        nullptr;

    /*
     * IWbemServices::ExecMethod has 7 parameters:
     *
     * objectPath
     * methodName
     * flags
     * context
     * inputParameters
     * outputParameters
     * callResult
     */

    HRESULT result =
        services->ExecMethod(
            objectPath,
            methodName,
            0,
            nullptr,
            nullptr,
            &output,
            nullptr);

    SysFreeString(
        objectPath);

    SysFreeString(
        methodName);

    if (FAILED(result) ||
        output == nullptr)
    {
        std::cout
            << "GetSystemPartition failed. HRESULT: 0x"
            << std::hex
            << result
            << std::dec
            << std::endl;

        return false;
    }


    // --------------------------------------------------------
    // Read the Partition output
    // --------------------------------------------------------

    VARIANT partition;

    VariantInit(
        &partition);

    result =
        output->Get(
            L"Partition",
            0,
            &partition,
            nullptr,
            nullptr);

    output->Release();

    if (FAILED(result) ||
        partition.vt != VT_BSTR ||
        partition.bstrVal == nullptr)
    {
        std::cout
            << "Could not read BCD system partition. HRESULT: 0x"
            << std::hex
            << result
            << std::dec
            << std::endl;

        VariantClear(
            &partition);

        return false;
    }

    partitionPath =
        partition.bstrVal;

    VariantClear(
        &partition);

    std::wcout
        << L"BCD System Partition: "
        << partitionPath
        << std::endl;

    return true;
}


// ------------------------------------------------------------
// Find physical disk belonging to a BCD device path
//
// Example:
//
// BCD path:
// \Device\HarddiskVolume2
//
// We enumerate Windows volumes and compare the
// DOS device name returned by QueryDosDeviceW().
// ------------------------------------------------------------

bool getPhysicalDiskFromBcdPath(
    std::wstring bcdPath,
    DWORD& diskNumber,
    DWORD& partitionNumber,
    std::wstring& volumePath)
{
    HANDLE searchHandle =
        FindFirstVolumeW(
            nullptr,
            0);

    // The above call is only used to initialize the
    // variable safely. The real enumeration starts below.
    if (searchHandle != INVALID_HANDLE_VALUE)
    {
        FindVolumeClose(
            searchHandle);
    }


    wchar_t volumeName[MAX_PATH] = {};

    searchHandle =
        FindFirstVolumeW(
            volumeName,
            MAX_PATH);

    if (searchHandle ==
        INVALID_HANDLE_VALUE)
    {
        return false;
    }

    bool found = false;

    do
    {
        HANDLE volumeHandle =
            CreateFileW(
                volumeName,
                0,
                FILE_SHARE_READ |
                    FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);

        if (volumeHandle !=
            INVALID_HANDLE_VALUE)
        {
            /*
             * FindFirstVolumeW gives:
             *
             * \\?\Volume{GUID}\
             *
             * QueryDosDeviceW expects:
             *
             * Volume{GUID}
             */

            wchar_t deviceName[512] = {};

            DWORD length =
                QueryDosDeviceW(
                    volumeName + 4,
                    deviceName,
                    512);

            if (length > 0)
            {
                std::wstring currentDevice =
                    deviceName;

                if (_wcsicmp(
                        currentDevice.c_str(),
                        bcdPath.c_str()) == 0)
                {
                    STORAGE_DEVICE_NUMBER deviceNumber = {};

                    DWORD bytesReturned = 0;

                    BOOL result =
                        DeviceIoControl(
                            volumeHandle,
                            IOCTL_STORAGE_GET_DEVICE_NUMBER,
                            nullptr,
                            0,
                            &deviceNumber,
                            sizeof(deviceNumber),
                            &bytesReturned,
                            nullptr);

                    if (result)
                    {
                        diskNumber =
                            deviceNumber.DeviceNumber;

                        partitionNumber =
                            deviceNumber.PartitionNumber;

                        volumePath =
                            volumeName;

                        found = true;
                    }
                }
            }

            CloseHandle(
                volumeHandle);
        }

        if (found)
        {
            break;
        }

    } while (
        FindNextVolumeW(
            searchHandle,
            volumeName,
            MAX_PATH));

    FindVolumeClose(
        searchHandle);

    return found;
}


// ============================================================
// WindowsBootChecker
// ============================================================

BootInfo WindowsBootChecker::checkBootInfo()
{
    // --------------------------------------------------------
    // 1. Detect UEFI / BIOS
    // --------------------------------------------------------

    FIRMWARE_TYPE firmwareType;

    if (!GetFirmwareType(
            &firmwareType))
    {
        return BootInfo(
            "",
            "",
            "",
            "",
            false,
            false);
    }

    bool isUefi =
        firmwareType ==
        FirmwareTypeUefi;


    std::cout
        << "UEFI: "
        << (isUefi ? 1 : 0)
        << std::endl;


    // --------------------------------------------------------
    // 2. Find Windows directory
    // --------------------------------------------------------

    std::wstring windowsDirectory;

    if (!getWindowsDirectory(
            windowsDirectory))
    {
        return BootInfo(
            "",
            "",
            "",
            "",
            isUefi,
            false);
    }

    std::wcout
        << L"Windows Directory: "
        << windowsDirectory
        << std::endl;


    // --------------------------------------------------------
    // 3. Find Windows drive
    // --------------------------------------------------------

    std::wstring windowsDrive;

    if (!getWindowsDrive(
            windowsDirectory,
            windowsDrive))
    {
        return BootInfo(
            "",
            "",
            "",
            "",
            isUefi,
            false);
    }

    std::wcout
        << L"Windows Drive: "
        << windowsDrive
        << std::endl;


    // --------------------------------------------------------
    // 4. Find Windows volume GUID
    // --------------------------------------------------------

    std::wstring systemVolumeGuid;

    if (!getVolumeGuid(
            windowsDrive,
            systemVolumeGuid))
    {
        return BootInfo(
            "",
            "",
            "",
            "",
            isUefi,
            false);
    }

    std::wcout
        << L"Windows Volume: "
        << systemVolumeGuid
        << std::endl;


    // --------------------------------------------------------
    // 5. Find Windows physical disk
    // --------------------------------------------------------

    DWORD systemDiskNumber = 0;
    DWORD systemPartitionNumber = 0;

    if (!getPhysicalDisk(
            windowsDrive,
            systemDiskNumber,
            systemPartitionNumber))
    {
        return BootInfo(
            "",
            "",
            "",
            "",
            isUefi,
            false);
    }

    std::cout
        << "Windows Physical Disk: PhysicalDrive"
        << systemDiskNumber
        << std::endl;

    std::cout
        << "Windows Partition Number: "
        << systemPartitionNumber
        << std::endl;


    // --------------------------------------------------------
    // 6. Initialize WMI
    // --------------------------------------------------------

    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;

    if (!initializeWmi(
            locator,
            services))
    {
        return BootInfo(
            "",
            "",
            "",
            "",
            isUefi,
            false);
    }

    std::cout
        << "WMI initialized successfully."
        << std::endl;


    // --------------------------------------------------------
    // 7. Get actual boot/system partition from BCD
    // --------------------------------------------------------

    std::wstring bootPartitionDevice;

    bool bootPartitionFound =
        getSystemPartitionFromBcd(
            services,
            bootPartitionDevice);


    // WMI is no longer needed.

    services->Release();
    locator->Release();

    CoUninitialize();


    // --------------------------------------------------------
    // 8. BCD failed
    // --------------------------------------------------------

    if (!bootPartitionFound)
    {
        std::cout
            << "Boot partition was NOT found."
            << std::endl;

        return BootInfo(
            "",
            "",
            "",
            "",
            isUefi,
            false);
    }


    // --------------------------------------------------------
    // 9. Map BCD boot partition to physical disk
    // --------------------------------------------------------

    DWORD bootDiskNumber = 0;
    DWORD bootPartitionNumber = 0;

    std::wstring bootVolumePath;

    if (!getPhysicalDiskFromBcdPath(
            bootPartitionDevice,
            bootDiskNumber,
            bootPartitionNumber,
            bootVolumePath))
    {
        std::wcout
            << L"Could not map BCD partition: "
            << bootPartitionDevice
            << std::endl;

        return BootInfo(
            "",
            "",
            "",
            "",
            isUefi,
            false);
    }

    std::cout
        << "Boot Physical Disk: PhysicalDrive"
        << bootDiskNumber
        << std::endl;

    std::cout
        << "Boot Partition Number: "
        << bootPartitionNumber
        << std::endl;


    // --------------------------------------------------------
    // 10. Create physical disk IDs
    // --------------------------------------------------------

    std::string bootDiskId =
        "\\\\.\\PhysicalDrive" +
        std::to_string(
            bootDiskNumber);

    std::string systemDiskId =
        "\\\\.\\PhysicalDrive" +
        std::to_string(
            systemDiskNumber);


    // --------------------------------------------------------
    // 11. Create partition paths
    // --------------------------------------------------------

    std::string bootPartitionPath =
        wideToString(
            bootVolumePath);

    std::string systemPartitionPath =
        wideToString(
            systemVolumeGuid);


    // --------------------------------------------------------
    // 12. Final result
    // --------------------------------------------------------

    std::cout
        << "Windows boot information detected successfully."
        << std::endl;

    return BootInfo(
        bootDiskId,
        systemDiskId,
        bootPartitionPath,
        systemPartitionPath,
        isUefi,
        true);
}