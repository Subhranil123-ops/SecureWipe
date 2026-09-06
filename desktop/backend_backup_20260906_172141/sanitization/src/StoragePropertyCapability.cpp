#include "StoragePropertyCapability.h"

#include <Windows.h>
#include <winioctl.h>

#include <iostream>


bool queryStorageProperties(
    HANDLE deviceHandle)
{
    if (deviceHandle == INVALID_HANDLE_VALUE)
        return false;


    STORAGE_PROPERTY_QUERY query{};

    query.PropertyId =
        StorageDeviceProperty;

    query.QueryType =
        PropertyStandardQuery;


    BYTE buffer[1024]{};

    DWORD bytesReturned = 0;


    BOOL success =
        DeviceIoControl(
            deviceHandle,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            buffer,
            sizeof(buffer),
            &bytesReturned,
            nullptr);


    if (!success)
    {
        std::cout
            << "\nStorage property query failed.\n"
            << "Windows error: "
            << GetLastError()
            << '\n';

        return false;
    }


    if (bytesReturned <
        sizeof(STORAGE_DEVICE_DESCRIPTOR))
    {
        std::cout
            << "\nStorage property response is too small.\n";

        return false;
    }


    auto* descriptor =
        reinterpret_cast<
            STORAGE_DEVICE_DESCRIPTOR*>(
                buffer);


    std::cout
        << "\nWindows Storage Property Information\n";


    // --------------------------------------------------------
    // Bus Type
    // --------------------------------------------------------

    std::cout
        << "Bus Type: ";

    switch (descriptor->BusType)
    {
    case BusTypeUsb:
        std::cout << "USB";
        break;

    case BusTypeScsi:
        std::cout << "SCSI";
        break;

    case BusTypeAta:
        std::cout << "ATA";
        break;

    case BusTypeSata:
        std::cout << "SATA";
        break;

    case BusTypeNvme:
        std::cout << "NVMe";
        break;

    default:
        std::cout << "Other / Unknown";
        break;
    }

    std::cout << '\n';


    // --------------------------------------------------------
    // Device Type
    // --------------------------------------------------------

    std::cout
        << "Device Type: "
        << static_cast<int>(
               descriptor->DeviceType)
        << '\n';


    // --------------------------------------------------------
    // Removable Media
    // --------------------------------------------------------

    std::cout
        << "Removable Media: "
        << (descriptor->RemovableMedia
                ? "YES"
                : "NO")
        << '\n';


    // --------------------------------------------------------
    // Vendor ID
    // --------------------------------------------------------

    if (descriptor->VendorIdOffset != 0 &&
        descriptor->VendorIdOffset < bytesReturned)
    {
        const char* vendor =
            reinterpret_cast<const char*>(
                buffer +
                descriptor->VendorIdOffset);

        std::cout
            << "Windows Vendor ID: "
            << vendor
            << '\n';
    }


    // --------------------------------------------------------
    // Product ID
    // --------------------------------------------------------

    if (descriptor->ProductIdOffset != 0 &&
        descriptor->ProductIdOffset < bytesReturned)
    {
        const char* product =
            reinterpret_cast<const char*>(
                buffer +
                descriptor->ProductIdOffset);

        std::cout
            << "Windows Product ID: "
            << product
            << '\n';
    }


    return true;
}