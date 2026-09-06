#pragma once

#include <string>
#include <cstdint>

class StorageDevice
{
private:
    std::string model_;
    std::string serialNumber_;
    std::uint64_t capacityBytes_;
    std::string deviceId_;
    std::string interfaceType_;
    bool isSystemDisk_;
    bool isRemovable_;
    bool hasSeekPenalty_;

public:
    StorageDevice(
        std::string deviceId,
        std::string model,
        std::string serialNumber,
        std::uint64_t capacityBytes,
        std::string interfaceType,
        bool isSystemDisk,
        bool isRemovable,
        bool hasSeekPenalty);

    const std::string &getModel() const;
    const std::string &getDeviceId() const;
    const std::string &getSerialNumber() const;
    const std::string &getInterfaceType() const;
    bool isSystemDisk() const;
    std::uint64_t getCapacityBytes() const;
    bool isRemovable() const;
    bool hasSeekPenalty() const;
};