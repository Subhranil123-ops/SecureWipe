#pragma once

#include <cstdint>
#include <string>

struct DeviceIdentity
{
    std::string deviceId;
    std::string model;
    std::string serialNumber;
    std::uint64_t capacityBytes{0};

    // True when the identity was supplied by an external
    // request and is therefore bound to the device serial.
    bool serialBound{false};
};