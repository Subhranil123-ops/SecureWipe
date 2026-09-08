#pragma once

#include "../../storage/include/StorageDevice.h"
#include "DeviceIdentity.h"
#include <string>
#include <vector>
#include "SafetyResult.h"

class SafetyEngine
{
private:
    DeviceIdentity expectedTarget_;
    bool hasExpectedTarget_ = false;

    /*
     * When true, the expected target came from a
     * sanitization request and serialNumber is the
     * stable physical-device identity.
     *
     * PhysicalDriveN must NOT be treated as the identity
     * because Windows can assign another PhysicalDriveN
     * after reconnecting the same physical device.
     */
    bool expectedTargetUsesSerial_ = false;

    // Find the previously selected device
    // in the freshly discovered device list.
    bool findTarget(
        const std::vector<StorageDevice> &devices,
        StorageDevice &target);

    // Check 1:
    // Is the selected device the disk containing
    // the currently running Windows installation?
    bool checkSystemDisk(
        const StorageDevice &device);

    // Check 2:
    // Does the current boot process depend on this device?
    bool checkBootDependency(
        const StorageDevice &device);

    // Check 3:
    // Are any volumes/partitions of this device currently
    // mounted or actively in use?
    bool checkMountedVolume(
        const StorageDevice &device);

    // Check 4:
    // Is this actually a physical storage device that
    // SecureWipe is allowed to sanitize?
    bool checkPhysicalDevice(
        const StorageDevice &device);

    // Check 5:
    // Does the device still match the exact device selected
    // for this sanitization request?
    bool checkTargetIdentity(
        const StorageDevice &device);

public:
    SafetyResult evaluateWithResult(
        const StorageDevice &device);

    // Run enabled safety checks on the target.
    bool evaluate(
        const StorageDevice &device);

    // Legacy/local target binding.
    // This compares the original physical target information.
    void setExpectedTarget(
        const StorageDevice &device);

    // Request-bound identity.
    //
    // The supplied serial number becomes the stable identity.
    // PhysicalDriveN is deliberately ignored during identity
    // comparison so reconnecting through USB does not break
    // recognition of the same physical disk.
    void setExpectedTargetSerial(
        const std::string &serialNumber);

    // Find and validate the previously selected target
    // in a freshly discovered device list.
    bool validateTarget(
        const std::vector<StorageDevice> &devices,
        StorageDevice &target);
};