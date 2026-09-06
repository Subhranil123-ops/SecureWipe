#include "StorageManager.h"

#include <iostream>

int main()
{
    StorageManager manager;

    manager.discoverDevices();

    const auto &devices = manager.getDevices();

    std::cout << "Detected devices: "
              << devices.size()
              << "\n";

    return 0;
}