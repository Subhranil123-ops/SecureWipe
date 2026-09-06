#pragma once
#include "../../storage/include/StorageDevice.h"
#include "DeviceIdentity.h"
#include <vector>
#include "SafetyResult.h"

class SafetyEngine
{
private:
    DeviceIdentity expectedTarget_;
    bool hasExpectedTarget_ = {false};

    // Find the previously selected device
    // in the freshly discovered device list.
    bool findTarget(
        const std::vector<StorageDevice> &devices, StorageDevice &target);

    // Check 1:
    // Is the selected device the disk containing
    // the currently running Windows installation?
    bool checkSystemDisk(const StorageDevice &device);

    // Check 2:
    // Does the current boot process depend on this device?
    // Example: current EFI / boot files.
    bool checkBootDependency(const StorageDevice &device);

    // Check 3:
    // Are any volumes/partitions of this device currently
    // mounted or actively in use?
    bool checkMountedVolume(const StorageDevice &device);

    // Check 5:
    // Is this actually a physical storage device that
    // SecureWipe is allowed to sanitize?
    bool checkPhysicalDevice(const StorageDevice &device);

    // Check 6:
    // Does the device still match the exact device selected
    // by the user?
    bool checkTargetIdentity(const StorageDevice &device);

public:

    SafetyResult evaluateWithResult(const StorageDevice &device);

    // Run enabled safety checks on the target.
    bool evaluate(const StorageDevice &device);

    // Save the device selected by the user.
    void setExpectedTarget(const StorageDevice &device);

    // Find and validate the previously selected
    // device in a freshly discovered device list.
    bool validateTarget(const std::vector<StorageDevice> &devices, StorageDevice &target);
};