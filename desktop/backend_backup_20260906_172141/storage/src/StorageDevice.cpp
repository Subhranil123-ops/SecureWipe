#include "StorageDevice.h"

StorageDevice::StorageDevice(
    std::string deviceId,
    std::string model,
    std::string serialNumber,
    std::uint64_t capacityBytes,
    std::string interfaceType,
    bool isSystemDisk,
    bool isRemovable,
    bool hasSeekPenalty

)
{
    model_ = model;
    serialNumber_ = serialNumber;
    capacityBytes_ = capacityBytes;
    deviceId_ = deviceId;
    interfaceType_ = interfaceType;
    isSystemDisk_ = isSystemDisk;
    isRemovable_=isRemovable;
    hasSeekPenalty_=hasSeekPenalty;
}

const std::string& StorageDevice::getModel() const
{
    return model_;
}

const std::string& StorageDevice::getDeviceId() const
{
    return deviceId_;
}

const std::string& StorageDevice::getSerialNumber() const
{
    return serialNumber_;
}

const std::string& StorageDevice::getInterfaceType() const
{
    return interfaceType_;
}

std::uint64_t StorageDevice::getCapacityBytes() const
{
    return capacityBytes_;
}

bool StorageDevice::isSystemDisk() const
{
    return isSystemDisk_;
}

bool StorageDevice::isRemovable() const{
  return isRemovable_;
};

bool StorageDevice:: hasSeekPenalty() const{return hasSeekPenalty_; };


