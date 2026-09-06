#pragma once

#include <vector>

#include "StorageDevice.h"

class StorageService
{
public:
    StorageService() = default;

    std::vector<StorageDevice> discoverDevices();
};