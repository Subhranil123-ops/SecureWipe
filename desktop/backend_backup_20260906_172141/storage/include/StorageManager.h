#pragma once

#include <vector>
#include "StorageDevice.h"

class StorageManager
{
private:
    std::vector<StorageDevice> devices_;

public:
    void addDevice(const StorageDevice &device);
    const std::vector<StorageDevice> &getDevices() const;
    void clearDevices();
    void discoverDevices();
};
