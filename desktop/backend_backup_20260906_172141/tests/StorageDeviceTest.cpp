#include "../storage/include/StorageDevice.h"

#include <iostream>

int main()
{
    StorageDevice device(
        "device001",
        "Test SSD",
        "SN123456789",
        1000000000,
        "NVMe",
        false,   // isSystemDisk
        false,   // isRemovable
        false    // hasSeekPenalty
    );

    std::cout << "Model: "
              << device.getModel()
              << "\n";

    std::cout << "Device ID: "
              << device.getDeviceId()
              << "\n";

    std::cout << "Serial: "
              << device.getSerialNumber()
              << "\n";

    std::cout << "Capacity: "
              << device.getCapacityBytes()
              << "\n";

    std::cout << "Interface: "
              << device.getInterfaceType()
              << "\n";

    std::cout << "System Disk: "
              << (device.isSystemDisk() ? "YES" : "NO")
              << "\n";

    std::cout << "Removable: "
              << (device.isRemovable() ? "YES" : "NO")
              << "\n";

    std::cout << "Seek Penalty: "
              << (device.hasSeekPenalty() ? "YES" : "NO")
              << "\n";

    return 0;
}