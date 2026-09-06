#pragma once

#include "StorageDevice.h"
#include "vector"

class WindowsStorageDiscovery
{

public:
    std::vector<StorageDevice> discover();
    
};