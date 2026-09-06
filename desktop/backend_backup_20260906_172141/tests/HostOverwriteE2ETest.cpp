#include <Windows.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"
#include "SafetyEngine.h"
#include "SafetyResult.h"
#include "SanitizationCapability.h"
#include "SanitizationEngine.h"
#include "SanitizationMethod.h"
#include "SanitizationResult.h"

static std::string methodToString(
    SanitizationMethod method)
{
    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
        return "NVMe Sanitize";

    case SanitizationMethod::AtaSanitize:
        return "ATA Sanitize";

    case SanitizationMethod::HostOverwrite:
        return "Host Overwrite";

    case SanitizationMethod::Unsupported:
        return "Unsupported";
    }

    return "Unknown";
}

static std::string readLine()
{
    std::string input;
    std::getline(std::cin, input);
    return input;
}

static bool parseDeviceIndex(
    const std::string& input,
    std::size_t deviceCount,
    std::size_t& index)
{
    try
    {
        std::size_t consumed = 0;

        const unsigned long long value =
            std::stoull(
                input,
                &consumed);

        if (consumed != input.size())
            return false;

        if (value == 0 ||
            value > deviceCount)
        {
            return false;
        }

        index =
            static_cast<std::size_t>(
                value - 1);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

static void separator()
{
    std::cout
        << "\n============================================================\n";
}

static void printDevice(
    const StorageDevice& device,
    std::size_t number)
{
    std::cout
        << "\n[" << number << "]\n"
        << "  Device ID   : "
        << device.getDeviceId()
        << '\n'
        << "  Model       : "
        << device.getModel()
        << '\n'
        << "  Serial      : "
        << device.getSerialNumber()
        << '\n'
        << "  Interface   : "
        << device.getInterfaceType()
        << '\n'
        << "  Capacity    : "
        << device.getCapacityBytes()
        << " bytes\n"
        << "  System Disk : "
        << (device.isSystemDisk()
                ? "YES - BLOCKED"
                : "NO")
        << '\n'
        << "  Removable   : "
        << (device.isRemovable()
                ? "YES"
                : "NO")
        << '\n';
}

static bool runSafety(
    SafetyEngine& safetyEngine,
    const StorageDevice& device,
    SafetyResult& result)
{
    separator();

    std::cout
        << "STEP 2 - SAFETY ENGINE\n"
        << "------------------------------------------------------------\n";

    safetyEngine.setExpectedTarget(device);

    result =
        safetyEngine.evaluateWithResult(
            device);

    std::cout
        << "\nSafety Decision : "
        << result.decision
        << '\n'
        << "Safety Summary  : "
        << result.summary
        << '\n';

    std::cout
        << "\nIndividual Checks\n"
        << "-----------------\n";

    bool allPassed = true;

    for (const auto& check : result.checks)
    {
        std::cout
            << "\n"
            << (check.passed
                    ? "[PASS] "
                    : "[FAIL] ")
            << check.checkName
            << '\n'
            << "       "
            << check.message
            << '\n';

        if (!check.passed)
            allPassed = false;
    }

    if (!result.isOverallSafe ||
        !allPassed)
    {
        std::cout
            << "\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "SANITIZATION BLOCKED\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

        return false;
    }

    std::cout
        << "\n[SAFETY PASS]\n";

    return true;
}

static bool confirmTarget(
    const StorageDevice& device)
{
    separator();

    std::cout
        << "DESTRUCTIVE OPERATION CONFIRMATION\n"
        << "------------------------------------------------------------\n\n";

    std::cout
        << "WARNING: THIS OPERATION IS DESTRUCTIVE.\n\n"
        << "The entire physical device will be overwritten with 0x00.\n"
        << "All existing data will be destroyed.\n"
        << "This operation cannot be undone.\n\n";

    std::cout
        << "Target Device\n"
        << "-------------\n"
        << "Device ID : "
        << device.getDeviceId()
        << '\n'
        << "Model     : "
        << device.getModel()
        << '\n'
        << "Serial    : "
        << device.getSerialNumber()
        << '\n'
        << "Interface : "
        << device.getInterfaceType()
        << '\n'
        << "Capacity  : "
        << device.getCapacityBytes()
        << " bytes\n\n";

    std::cout
        << "Enter the exact target serial number: ";

    const std::string serial =
        readLine();

    if (serial !=
        device.getSerialNumber())
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Serial number does not match.\n";

        return false;
    }

    std::cout
        << "\nSerial confirmation matched.\n";

    std::cout
        << "Type YES to continue: ";

    const std::string confirmation =
        readLine();

    if (confirmation != "YES")
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Destructive operation was not authorized.\n";

        return false;
    }

    return true;
}

int main()
{
    separator();

    std::cout
        << "        SECUREWIPE HOST OVERWRITE E2E TEST\n"
        << "        REAL PHYSICAL DEVICE TEST\n";

    separator();

    std::cout
        << "\nIMPORTANT:\n"
        << "This test permanently destroys data.\n"
        << "Use ONLY a disposable/sacrificial device.\n"
        << "NEVER select the Windows system disk.\n";

    // =========================================================
    // STEP 1 - DISCOVERY
    // =========================================================

    separator();

    std::cout
        << "STEP 1 - DEVICE DISCOVERY\n"
        << "------------------------------------------------------------\n";

    WindowsStorageDiscovery discovery;

    std::cout
        << "Discovering physical storage devices...\n";

    std::vector<StorageDevice> devices =
        discovery.discover();

    if (devices.empty())
    {
        std::cout
            << "\n[FAIL]\n"
            << "No physical storage devices detected.\n";

        return 1;
    }

    std::cout
        << "\nDetected "
        << devices.size()
        << " physical storage device(s).\n";

    for (std::size_t i = 0;
         i < devices.size();
         ++i)
    {
        printDevice(
            devices[i],
            i + 1);
    }

    // =========================================================
    // TARGET SELECTION
    // =========================================================

    separator();

    std::cout
        << "TARGET SELECTION\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "\nEnter the number of the SACRIFICIAL device: ";

    const std::string input =
        readLine();

    std::size_t selectedIndex = 0;

    if (!parseDeviceIndex(
            input,
            devices.size(),
            selectedIndex))
    {
        std::cout
            << "\n[FAIL]\n"
            << "Invalid device selection.\n";

        return 1;
    }

    StorageDevice selectedDevice =
        devices[selectedIndex];

    printDevice(
        selectedDevice,
        selectedIndex + 1);

    // =========================================================
    // HARD SAFETY GUARDS
    // =========================================================

    if (selectedDevice.isSystemDisk())
    {
        std::cout
            << "\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "CRITICAL SAFETY BLOCK\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "Selected device is the Windows system disk.\n"
            << "Operation will NOT continue.\n";

        return 2;
    }

    if (selectedDevice.getDeviceId().empty())
    {
        std::cout
            << "\n[FAIL]\n"
            << "Device ID is empty.\n";

        return 1;
    }

    if (selectedDevice.getSerialNumber().empty())
    {
        std::cout
            << "\n[FAIL]\n"
            << "Serial number is empty.\n";

        return 1;
    }

    if (selectedDevice.getCapacityBytes() == 0)
    {
        std::cout
            << "\n[FAIL]\n"
            << "Device capacity is zero.\n";

        return 1;
    }

    // =========================================================
    // STEP 2 - SAFETY
    // =========================================================

    SafetyEngine safetyEngine;
    SafetyResult safetyResult;

    if (!runSafety(
            safetyEngine,
            selectedDevice,
            safetyResult))
    {
        return 2;
    }

    // =========================================================
    // STEP 3 - CAPABILITY
    // =========================================================

    separator();

    std::cout
        << "STEP 3 - CAPABILITY & METHOD SELECTION\n"
        << "------------------------------------------------------------\n";

    SanitizationCapability capability =
        detectSanitizationCapability(
            selectedDevice);

    std::cout
        << "\nCapability Summary\n"
        << "------------------\n"
        << "USB Device             : "
        << (capability.isUsbDevice
                ? "YES"
                : "NO")
        << '\n'
        << "Storage Property Query : "
        << (capability.storagePropertyQueryAvailable
                ? "AVAILABLE"
                : "NOT AVAILABLE")
        << '\n'
        << "SCSI Path              : "
        << (capability.scsiPathAvailable
                ? "AVAILABLE"
                : "NOT AVAILABLE")
        << '\n'
        << "NVMe Identify          : "
        << (capability.nvmeIdentifyAvailable
                ? "AVAILABLE"
                : "NOT AVAILABLE")
        << '\n'
        << "ATA Identify           : "
        << (capability.ataIdentifyAvailable
                ? "AVAILABLE"
                : "NOT AVAILABLE")
        << '\n';

    SanitizationEngine sanitizationEngine;

    const SanitizationMethod method =
        sanitizationEngine.selectMethod(
            selectedDevice,
            capability);

    std::cout
        << "\nSelected Sanitization Method: "
        << methodToString(method)
        << '\n';

    if (method !=
        SanitizationMethod::HostOverwrite)
    {
        std::cout
            << "\n[TEST STOPPED]\n"
            << "This test is specifically for Host Overwrite.\n"
            << "No destructive operation was started.\n";

        return 3;
    }

    std::cout
        << "\n[PASS]\n"
        << "Host Overwrite selected.\n";

    // =========================================================
    // STEP 4 - CONFIRMATION
    // =========================================================

    if (!confirmTarget(
            selectedDevice))
    {
        return 4;
    }

    // =========================================================
    // STEP 5 - FRESH DISCOVERY
    // =========================================================

    separator();

    std::cout
        << "STEP 5 - FINAL TARGET REVALIDATION\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Performing fresh device discovery...\n";

    std::vector<StorageDevice> freshDevices =
        discovery.discover();

    if (freshDevices.empty())
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Fresh discovery returned no devices.\n";

        return 5;
    }

    if (!safetyEngine.validateTarget(
            freshDevices,
            selectedDevice))
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Original target could not be safely revalidated.\n";

        return 5;
    }

    StorageDevice* freshTarget = nullptr;

    for (auto& device : freshDevices)
    {
        if (device.getDeviceId() ==
            selectedDevice.getDeviceId())
        {
            freshTarget = &device;
            break;
        }
    }

    if (freshTarget == nullptr)
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Fresh target was not found.\n";

        return 5;
    }

    selectedDevice =
        *freshTarget;

    if (selectedDevice.isSystemDisk())
    {
        std::cout
            << "\n[CRITICAL BLOCK]\n"
            << "Target is now detected as system disk.\n";

        return 5;
    }

    SafetyResult finalSafetyResult =
        safetyEngine.evaluateWithResult(
            selectedDevice);

    if (!finalSafetyResult.isOverallSafe)
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Final safety validation failed.\n"
            << finalSafetyResult.summary
            << '\n';

        return 5;
    }

    std::cout
        << "\n[FINAL VALIDATION PASS]\n"
        << "Target identity and safety state remain valid.\n";

    // =========================================================
    // STEP 6 - FINAL WARNING
    // =========================================================

    separator();

    std::cout
        << "STEP 6 - FINAL DESTRUCTIVE AUTHORIZATION\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "\nTARGET:\n"
        << "Device ID : "
        << selectedDevice.getDeviceId()
        << '\n'
        << "Model     : "
        << selectedDevice.getModel()
        << '\n'
        << "Serial    : "
        << selectedDevice.getSerialNumber()
        << '\n'
        << "Capacity  : "
        << selectedDevice.getCapacityBytes()
        << " bytes\n"
        << "Method    : Host Overwrite\n"
        << "Pattern   : 0x00\n\n";

    std::cout
        << "Type START WIPE to permanently erase this device: ";

    const std::string wipeConfirmation =
        readLine();

    if (wipeConfirmation !=
        "START WIPE")
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Final destructive authorization failed.\n";

        return 6;
    }

    // =========================================================
    // STEP 7 - EXECUTION
    // =========================================================

    separator();

    std::cout
        << "STEP 7 - HOST OVERWRITE EXECUTION\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "\nSTARTING DESTRUCTIVE OPERATION.\n"
        << "DO NOT DISCONNECT THE DEVICE.\n"
        << "DO NOT POWER OFF THE COMPUTER.\n\n";

    const auto start =
        GetTickCount64();

    SecureWipe::SanitizationResult result =
        sanitizationEngine.sanitize(
            selectedDevice,
            finalSafetyResult);

    const auto end =
        GetTickCount64();

    std::cout
        << "\nExecution time: "
        << (end - start)
        << " ms\n";

    // =========================================================
    // STEP 8 - RESULT
    // =========================================================

    separator();

    std::cout
        << "STEP 8 - FINAL RESULT\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Status              : ";

    switch (result.status)
    {
    case SecureWipe::SanitizationStatus::COMPLETED:
        std::cout << "COMPLETED";
        break;

    case SecureWipe::SanitizationStatus::FAILED:
        std::cout << "FAILED";
        break;

    case SecureWipe::SanitizationStatus::IN_PROGRESS:
        std::cout << "IN PROGRESS";
        break;
    }

    std::cout
        << '\n'
        << "Message             : "
        << result.message
        << '\n'
        << "Bytes Processed     : "
        << result.bytesProcessed
        << '\n';

    std::cout
        << "Verification        : ";

    switch (result.verificationStatus)
    {
    case SecureWipe::VerificationStatus::PASSED:
        std::cout << "PASSED";
        break;

    case SecureWipe::VerificationStatus::FAILED:
        std::cout << "FAILED";
        break;

    case SecureWipe::VerificationStatus::IN_PROGRESS:
        std::cout << "IN PROGRESS";
        break;

    case SecureWipe::VerificationStatus::NOT_PERFORMED:
        std::cout << "NOT PERFORMED";
        break;
    }

    std::cout
        << '\n'
        << "Verification Samples : "
        << result.verificationSamples
        << '\n'
        << "Bytes Verified       : "
        << result.bytesVerified
        << '\n'
        << "Verification Message : "
        << result.verificationMessage
        << '\n'
        << "Duration             : "
        << result.operationDurationMs
        << " ms\n";

    // =========================================================
    // FINAL DECISION
    // =========================================================

    separator();

    const bool finalPass =
        result.status ==
            SecureWipe::SanitizationStatus::COMPLETED &&
        result.verificationStatus ==
            SecureWipe::VerificationStatus::PASSED;

    if (finalPass)
    {
        std::cout
            << "\n"
            << "============================================================\n"
            << "                 E2E TEST PASSED\n"
            << "============================================================\n"
            << "\n"
            << "Host Overwrite completed successfully.\n"
            << "Post-write verification passed.\n";
    }
    else
    {
        std::cout
            << "\n"
            << "============================================================\n"
            << "                 E2E TEST FAILED\n"
            << "============================================================\n"
            << "\n"
            << "The complete sanitization + verification criteria\n"
            << "were not satisfied.\n";
    }

    return finalPass ? 0 : 7;
}