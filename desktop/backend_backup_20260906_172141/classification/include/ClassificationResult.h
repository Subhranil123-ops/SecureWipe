#pragma once

enum class MediaType
{
    Unknown,
    HDD,
    SSD
};

enum class BusType
{
    Unkown,
    SATA,
    SAS,
    USB,
    NVMe
};

enum class DeviceType
{
    Unknown,
    Internal,
    Removable
};

struct ClassificationResult
{
    MediaType mediaType;
    BusType busType;
    DeviceType deviceType;
    bool isSystemDisk;
};