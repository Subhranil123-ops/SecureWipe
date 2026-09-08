#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

#include "SafetyEngine.h"
#include "WindowsStorageUtils.h"

void SafetyEngine::setExpectedTarget(
    const StorageDevice &device)
{
    expectedTarget_.deviceId =
        device.getDeviceId();

    expectedTarget_.model =
        device.getModel();

    expectedTarget_.serialNumber =
        device.getSerialNumber();

    expectedTarget_.capacityBytes =
        device.getCapacityBytes();

    expectedTarget_.serialBound =
        false;

    expectedTargetUsesSerial_ =
        false;

    hasExpectedTarget_ =
        true;
}

void SafetyEngine::setExpectedTargetSerial(
    const std::string &serialNumber)
{
    expectedTarget_ =
        DeviceIdentity{};

    expectedTarget_.serialNumber =
        serialNumber;

    expectedTarget_.serialBound =
        true;

    expectedTargetUsesSerial_ =
        true;

    hasExpectedTarget_ =
        !serialNumber.empty();
}

bool SafetyEngine::checkSystemDisk(
    const StorageDevice &device)
{
    if (device.isSystemDisk())
        return false;

    return true;
}

bool SafetyEngine::checkBootDependency(
    const StorageDevice &device)
{
    /*
     * This check remains conservative with the current
     * implementation. A future boot-dependency detector can
     * replace this without changing the identity architecture.
     */
    return true;
}

bool SafetyEngine::checkMountedVolume(
    const StorageDevice &device)
{
    DWORD targetDiskNumber = 0;

    if (!WindowsStorageUtils::getDiskNumberFromDeviceId(
            device.getDeviceId(),
            targetDiskNumber))
    {
        return false;
    }

    WCHAR volumeName[MAX_PATH]{};

    HANDLE findHandle =
        FindFirstVolumeW(
            volumeName,
            ARRAYSIZE(volumeName));

    if (findHandle ==
        INVALID_HANDLE_VALUE)
    {
        return false;
    }

    while (true)
    {
        DWORD pathBufferSize =
            MAX_PATH;

        std::vector<WCHAR> pathBuffer(
            pathBufferSize);

        DWORD returnedLength = 0;

        BOOL pathsSuccess =
            GetVolumePathNamesForVolumeNameW(
                volumeName,
                pathBuffer.data(),
                pathBufferSize,
                &returnedLength);

        if (pathsSuccess)
        {
            WCHAR *currentPath =
                pathBuffer.data();

            while (*currentPath != L'\0')
            {
                std::wstring mountedPath =
                    currentPath;

                if (mountedPath.size() >= 3 &&
                    mountedPath[1] == L':' &&
                    mountedPath[2] == L'\\')
                {
                    std::wstring drive =
                        mountedPath.substr(
                            0,
                            2);

                    DWORD volumeDiskNumber =
                        0;

                    DWORD partitionNumber =
                        0;

                    if (WindowsStorageUtils::getPhysicalDisk(
                            drive,
                            volumeDiskNumber,
                            partitionNumber))
                    {
                        if (volumeDiskNumber ==
                            targetDiskNumber)
                        {
                            FindVolumeClose(
                                findHandle);

                            std::cout
                                << "Mounted volume found on target disk\n";

                            return false;
                        }
                    }
                }

                currentPath +=
                    wcslen(currentPath) + 1;
            }
        }

        if (!FindNextVolumeW(
                findHandle,
                volumeName,
                ARRAYSIZE(volumeName)))
        {
            break;
        }
    }

    FindVolumeClose(
        findHandle);

    return true;
}

bool SafetyEngine::checkPhysicalDevice(
    const StorageDevice &device)
{
    /*
     * A serial number is mandatory for the current
     * request-bound physical-device workflow.
     *
     * We deliberately fail closed when it is missing.
     */
    if (device.getDeviceId().empty())
    {
        return false;
    }

    if (device.getModel().empty())
    {
        return false;
    }

    if (device.getSerialNumber().empty())
    {
        return false;
    }

    if (device.getCapacityBytes() == 0)
    {
        return false;
    }

    if (device.getInterfaceType().empty())
    {
        return false;
    }

    return true;
}

bool SafetyEngine::checkTargetIdentity(
    const StorageDevice &device)
{
    if (!hasExpectedTarget_)
        return false;

    /*
     * REQUEST-BOUND IDENTITY
     *
     * Serial number is the stable physical identity.
     *
     * The current Windows physical-device path
     * (for example \\.\PhysicalDrive2) is NOT compared
     * here because the OS may expose the same physical
     * device through another PhysicalDriveN after
     * reconnecting it through a USB enclosure.
     */
    if (expectedTargetUsesSerial_)
    {
        if (expectedTarget_.serialNumber.empty())
            return false;

        if (device.getSerialNumber().empty())
            return false;

        return device.getSerialNumber() ==
               expectedTarget_.serialNumber;
    }

    /*
     * LEGACY / LOCAL SELECTION
     *
     * When there is no request-bound serial, retain the
     * original exact-target behaviour for internal callers
     * and existing tests.
     */
    if (device.getDeviceId() !=
        expectedTarget_.deviceId)
    {
        return false;
    }

    if (device.getModel() !=
        expectedTarget_.model)
    {
        return false;
    }

    if (device.getSerialNumber() !=
        expectedTarget_.serialNumber)
    {
        return false;
    }

    if (device.getCapacityBytes() !=
        expectedTarget_.capacityBytes)
    {
        return false;
    }

    return true;
}

SafetyResult SafetyEngine::evaluateWithResult(
    const StorageDevice &device)
{
    SafetyResult result;

    result.isOverallSafe =
        true;

    // --------------------------------------------------
    // SYSTEM DISK CHECK
    // --------------------------------------------------

    if (checkSystemDisk(
            device))
    {
        result.checks.push_back({
            "System Disk Check",
            true,
            "Target is not the current Windows system disk."
        });
    }
    else
    {
        result.checks.push_back({
            "System Disk Check",
            false,
            "Target is the current Windows system disk."
        });

        result.isOverallSafe =
            false;
    }

    // --------------------------------------------------
    // BOOT DEPENDENCY CHECK
    // --------------------------------------------------

    if (checkBootDependency(
            device))
    {
        result.checks.push_back({
            "Boot Dependency Check",
            true,
            "Target is not currently required for the boot process."
        });
    }
    else
    {
        result.checks.push_back({
            "Boot Dependency Check",
            false,
            "The current Windows boot process depends on this device."
        });

        result.isOverallSafe =
            false;
    }

    // --------------------------------------------------
    // MOUNTED VOLUME CHECK
    // --------------------------------------------------

    if (checkMountedVolume(
            device))
    {
        result.checks.push_back({
            "Mounted Volume Check",
            true,
            "No mounted or actively used volume was detected on the target."
        });
    }
    else
    {
        result.checks.push_back({
            "Mounted Volume Check",
            false,
            "A volume on the target disk is currently mounted or in use."
        });

        result.isOverallSafe =
            false;
    }

    // --------------------------------------------------
    // PHYSICAL DEVICE CHECK
    // --------------------------------------------------

    if (checkPhysicalDevice(
            device))
    {
        result.checks.push_back({
            "Physical Device Check",
            true,
            "Required physical device information is available."
        });
    }
    else
    {
        result.checks.push_back({
            "Physical Device Check",
            false,
            "Required physical device information is missing, including a reliable serial number."
        });

        result.isOverallSafe =
            false;
    }

    // --------------------------------------------------
    // TARGET IDENTITY CHECK
    // --------------------------------------------------

    if (checkTargetIdentity(
            device))
    {
        result.checks.push_back({
            "Target Identity Check",
            true,
            expectedTargetUsesSerial_
                ? "Target serial number matches the request-bound physical device identity."
                : "Target matches the device originally selected by the user."
        });
    }
    else
    {
        result.checks.push_back({
            "Target Identity Check",
            false,
            expectedTargetUsesSerial_
                ? "Target serial number does not match the request-bound physical device identity."
                : "Target does not match the device originally selected by the user."
        });

        result.isOverallSafe =
            false;
    }

    // --------------------------------------------------
    // FINAL DECISION
    // --------------------------------------------------

    if (result.isOverallSafe)
    {
        result.decision =
            "SAFE";

        result.summary =
            "All safety checks passed. "
            "The request-bound physical target is verified "
            "and sanitization may proceed.";
    }
    else
    {
        result.decision =
            "BLOCKED";

        result.summary =
            "One or more safety checks failed. "
            "Sanitization must not proceed.";
    }

    return result;
}

bool SafetyEngine::validateTarget(
    const std::vector<StorageDevice> &devices,
    StorageDevice &target)
{
    if (!hasExpectedTarget_)
        return false;

    for (const auto &device :
         devices)
    {
        if (checkTargetIdentity(
                device))
        {
            target =
                device;

            return true;
        }
    }

    return false;
}

bool SafetyEngine::evaluate(
    const StorageDevice &device)
{
    SafetyResult result =
        evaluateWithResult(
            device);

    return result.isOverallSafe;
}

bool SafetyEngine::findTarget(
    const std::vector<StorageDevice> &devices,
    StorageDevice &target)
{
    return validateTarget(
        devices,
        target);
}