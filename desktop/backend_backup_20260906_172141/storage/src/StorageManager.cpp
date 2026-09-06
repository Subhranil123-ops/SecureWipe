#include "StorageManager.h"
#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"

void StorageManager::addDevice(const StorageDevice &device)
{
    devices_.push_back(device);
}

const std::vector<StorageDevice> &StorageManager::getDevices() const
{
    return devices_;
};

void StorageManager::clearDevices()
{
    devices_.clear();
};

void StorageManager::discoverDevices()
{
    clearDevices();

    WindowsStorageDiscovery discovery;

    std::vector<StorageDevice> discoveredDevices = discovery.discover();

    for (const auto &device : discoveredDevices)
    {
        addDevice(device);
    }
}
