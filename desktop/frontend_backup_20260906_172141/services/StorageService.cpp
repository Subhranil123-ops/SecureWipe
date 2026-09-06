#include "StorageService.h"

#include "StorageManager.h"

std::vector<StorageDevice> StorageService::discoverDevices()
{
    StorageManager manager;

    manager.discoverDevices();

    return manager.getDevices();
}