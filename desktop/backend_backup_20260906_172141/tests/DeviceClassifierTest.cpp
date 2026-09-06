#include "../discovery/include/WindowsStorageDiscovery.h"
#include "../classification/include/DeviceClassifier.h"

#include <iostream>
#include <string>
#include <vector>

static std::string mediaTypetoString(MediaType type)
{
    if (type == MediaType::HDD)
        return "HDD";
    else if (type == MediaType::SSD)
        return "SSD";
    else
        return "Unknown";
}

static std::string busTypetoString(BusType type)
{
    if (type == BusType::SATA)
        return "SATA";
    else if (type == BusType::SAS)
        return "SAS";
    else if (type == BusType::USB)
        return "USB";
    else if (type == BusType::NVMe)
        return "NVMe";
    else
        return "Unknown";
}

static std::string deviceTypetoString(DeviceType type)
{
    if (type == DeviceType::Removable)
        return "Removable";
    else if (type == DeviceType::Internal)
        return "Internal";
    else
        return "Unknown";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << " SecureWipe - Device Classification Test\n";
    std::cout << "========================================\n\n";

    WindowsStorageDiscovery discovery;
    DeviceClassifier classifier;

    std::vector<StorageDevice> devices = discovery.discover();

    if (devices.empty())
    {
        std::cout << "No storage devices detected\n";
        return 0;
    }

    std::cout << "Device Detected : " << devices.size() << "\n\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
    {
        const StorageDevice &device = devices[i];
        ClassificationResult result = classifier.classify(device);

        std::cout << "----------------------------------------\n";
        std::cout << "Device #" << i + 1 << "\n";
        std::cout << "----------------------------------------\n";

        std::cout << "Model : " << device.getModel() << "\n";
        std::cout << "Serial Number : " << device.getSerialNumber() << "\n";
        std::cout << "Interface : " << device.getInterfaceType() << "\n";
        std::cout << "System Disk : " << (device.isSystemDisk() ? "YES" : "NO") << "\n";
        std::cout << "Removable       : " << (device.isRemovable() ? "YES" : "NO") << "\n";
        std::cout << "Seek Penalty    : " << (device.hasSeekPenalty() ? "YES" : "NO") << "\n\n";

        std::cout << "CLASSIFICATION\n";

        std::cout << "Media Type  : " << mediaTypetoString(result.mediaType) << "\n";
        std::cout << "Bus Type    : " << busTypetoString(result.busType) << "\n";
        std::cout << "Device Type : " << deviceTypetoString(result.deviceType) << "\n";
    }

    std::cout << "========================================\n";
    std::cout << " CLASSIFICATION TEST COMPLETE\n";
    std::cout << "========================================\n";
    return 0;
}