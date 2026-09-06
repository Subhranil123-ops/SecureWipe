#pragma once

#include <string>

struct BootInfo
{
    std::string bootDeviceId_;
    std::string systemDiskId_;
    std::string bootPartitionPath_;
    std::string systemPartitionPath_;
    bool isUefiBoot_;
    bool isValid_;

public:
    BootInfo(
        std::string bootDiskId,
        std::string systemDiskId,
        std::string bootPartitionPath,
        std::string systemPartitionPath,
        bool isUefiBoot,
        bool isValid);

    const std::string &getBootDiskId() const;

    const std::string &getSystemDiskId() const;

    const std::string &getBootPartitionPath() const;

    const std::string &getSystemPartitionPath() const;

    bool isUefiBoot() const;

    bool isValid() const;


};