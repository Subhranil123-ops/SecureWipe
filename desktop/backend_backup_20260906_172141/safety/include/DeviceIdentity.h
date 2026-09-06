#pragma once
#include <string>
#include <cstdint>

struct DeviceIdentity
{
    std::string deviceId;
    std::string model;
    std::string serialNumber;
    std::uint64_t capacityBytes{0};
};
