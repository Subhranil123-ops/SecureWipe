#include "BootInfo.h"

BootInfo::BootInfo(
    std::string bootDiskId,
    std::string systemDiskId,
    std::string bootPartitionPath,
    std::string systemPartitionPath,
    bool isUefiBoot,
    bool isValid)
{
    bootDeviceId_ = bootDiskId;
    systemDiskId_ = systemDiskId;
    bootPartitionPath_ = bootPartitionPath;
    systemPartitionPath_ = systemPartitionPath;
    isUefiBoot_ = isUefiBoot;
    isValid_ = isValid;
}

const std::string &BootInfo::getBootDiskId() const
{
    return bootDeviceId_;
};

const std::string &BootInfo::getSystemDiskId() const
{
    return systemDiskId_;
};

const std::string &BootInfo::getBootPartitionPath() const
{
    return bootPartitionPath_;
};

const std::string &BootInfo::getSystemPartitionPath() const
{
    return systemPartitionPath_;
};

bool BootInfo::isUefiBoot() const
{
    return isUefiBoot_;
};

bool BootInfo::isValid() const
{
    return isValid_;
};
