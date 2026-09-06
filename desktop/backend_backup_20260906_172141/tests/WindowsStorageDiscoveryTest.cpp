#include "../storage/include/StorageDevice.h"
#include "../discovery/include/WindowsStorageDiscovery.h"

#include <iostream>
#include <vector>

int main()
{
    std::cout << "========================================\n";
    std::cout << "   SecureWipe - Storage Discovery Test\n";
    std::cout << "========================================\n\n";

    WindowsStorageDiscovery discovery;

    std::vector<StorageDevice> devices =
        discovery.discover();

    std::cout << "\n========================================\n";
    std::cout << "       DISCOVERY RESULT\n";
    std::cout << "========================================\n\n";

    if (devices.empty())
    {
        std::cout << "No storage devices detected.\n";
        return 0;
    }

    std::cout << "Devices detected: "
              << devices.size()
              << "\n\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
    {
        const StorageDevice& device = devices[i];

        std::cout << "----------------------------------------\n";
        std::cout << "Device #" << i + 1 << "\n";
        std::cout << "----------------------------------------\n";

        std::cout << "Device ID       : "
                  << device.getDeviceId()
                  << "\n";

        std::cout << "Model           : "
                  << device.getModel()
                  << "\n";

        std::cout << "Serial Number   : "
                  << device.getSerialNumber()
                  << "\n";

        std::cout << "Capacity        : "
                  << device.getCapacityBytes()
                  << " bytes\n";

        std::cout << "Interface       : "
                  << device.getInterfaceType()
                  << "\n";

        std::cout << "System Disk     : "
                  << (device.isSystemDisk() ? "YES" : "NO")
                  << "\n";

        std::cout << "Removable       : "
                  << (device.isRemovable() ? "YES" : "NO")
                  << "\n";

        std::cout << "Seek Penalty    : "
                  << (device.hasSeekPenalty() ? "YES" : "NO")
                  << "\n";

        std::cout << "\n";
    }

    std::cout << "========================================\n";
    std::cout << "        DISCOVERY TEST COMPLETE\n";
    std::cout << "========================================\n";

    return 0;
}