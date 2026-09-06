#include <Windows.h>

#include <iostream>

#include "NvmeCapability.h"
#include "SanitizationCapability.h"


int main()
{
    std::cout
        << "========================================\n"
        << " SecureWipe NVMe Capability Test\n"
        << "========================================\n";


    const char* devicePath =
        "\\\\.\\PhysicalDrive0";


    std::cout
        << "\nOpening: "
        << devicePath
        << "\n";


    HANDLE deviceHandle =
        CreateFileA(
            devicePath,

            0,

            FILE_SHARE_READ |
            FILE_SHARE_WRITE,

            nullptr,

            OPEN_EXISTING,

            0,

            nullptr);


    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        std::cout
            << "\nFailed to open NVMe device.\n";

        std::cout
            << "Windows error: "
            << GetLastError()
            << '\n';

        return 1;
    }


    SanitizationCapability capability;


    bool success =
        detectNvmeCapability(
            deviceHandle,
            capability);


    CloseHandle(
        deviceHandle);


    std::cout
        << "\n========================================\n";


    if (success)
    {
        std::cout
            << " NVMe CAPABILITY QUERY PASSED\n";
    }
    else
    {
        std::cout
            << " NVMe CAPABILITY QUERY FAILED\n";
    }


    std::cout
        << "========================================\n";


    return success ? 0 : 1;
}