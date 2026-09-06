#include <Windows.h>

#include <cstdint>
#include <iostream>
#include <limits>
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

static std::string methodToString(SanitizationMethod method)
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
            std::stoull(input, &consumed);

        if (consumed != input.size())
            return false;

        if (value == 0 || value > deviceCount)
            return false;

        index = static_cast<std::size_t>(value - 1);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static void printSeparator()
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
        << "  Device ID     : " << device.getDeviceId() << '\n'
        << "  Model         : " << device.getModel() << '\n'
        << "  Serial        : " << device.getSerialNumber() << '\n'
        << "  Interface     : " << device.getInterfaceType() << '\n'
        << "  Capacity      : " << device.getCapacityBytes()
        << " bytes\n"
        << "  System Disk   : "
        << (device.isSystemDisk() ? "YES - BLOCKED" : "NO") << '\n'
        << "  Removable     : "
        << (device.isRemovable() ? "YES" : "NO") << '\n';
}

static bool runSafetyValidation(
    SafetyEngine& safetyEngine,
    const StorageDevice& device,
    SafetyResult& safetyResult)
{
    printSeparator();

    std::cout
        << "STEP 2 - SAFETY ENGINE VALIDATION\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "Setting expected target identity...\n";

    safetyEngine.setExpectedTarget(device);

    std::cout
        << "Expected target set.\n";

    std::cout
        << "\nRunning all safety checks...\n";

    safetyResult =
        safetyEngine.evaluateWithResult(device);

    std::cout
        << "\nSafety Decision : "
        << safetyResult.decision << '\n';

    std::cout
        << "Safety Summary  : "
        << safetyResult.summary << '\n';

    std::cout
        << "\nIndividual Safety Checks\n"
        << "-----------------------\n";

    bool allPassed = true;

    for (const auto& check : safetyResult.checks)
    {
        std::cout
            << "\n"
            << (check.passed ? "[PASS] " : "[FAIL] ")
            << check.checkName << '\n';

        std::cout
            << "       "
            << check.message << '\n';

        if (!check.passed)
            allPassed = false;
    }

    if (!safetyResult.isOverallSafe || !allPassed)
    {
        std::cout
            << "\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "SANITIZATION BLOCKED\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "Safety Engine did not authorize this target.\n";

        return false;
    }

    std::cout
        << "\n[SAFETY PASS]\n"
        << "All required safety checks passed.\n";

    return true;
}

static bool confirmDestructiveOperation(
    const StorageDevice& device)
{
    printSeparator();

    std::cout
        << "STEP 4 - DESTRUCTIVE OPERATION CONFIRMATION\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "\nWARNING: THIS OPERATION IS DESTRUCTIVE.\n\n";

    std::cout
        << "The complete physical device will be overwritten with 0x00.\n"
        << "Existing data, partitions and filesystem contents will be destroyed.\n"
        << "This operation cannot be undone.\n\n";

    std::cout
        << "Target Device\n"
        << "-------------\n"
        << "Device ID : " << device.getDeviceId() << '\n'
        << "Model     : " << device.getModel() << '\n'
        << "Serial    : " << device.getSerialNumber() << '\n'
        << "Interface : " << device.getInterfaceType() << '\n'
        << "Capacity  : " << device.getCapacityBytes()
        << " bytes\n";

    std::cout
        << "\nThe following serial number must be entered exactly:\n\n"
        << "    "
        << device.getSerialNumber()
        << "\n\n";

    std::cout
        << "Enter serial number to confirm: ";

    const std::string enteredSerial = readLine();

    if (enteredSerial != device.getSerialNumber())
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Confirmation value does not match target serial number.\n"
            << "No destructive operation was started.\n";

        return false;
    }

    std::cout
        << "\nSerial confirmation matched.\n";

    std::cout
        << "\nType YES to authorize the destructive operation: ";

    const std::string finalConfirmation = readLine();

    if (finalConfirmation != "YES")
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Final confirmation was not received.\n"
            << "No destructive operation was started.\n";

        return false;
    }

    std::cout
        << "\n[CONFIRMED]\n"
        << "Destructive operation authorized by operator.\n";

    return true;
}

int main()
{
    printSeparator();

    std::cout
        << "        SECUREWIPE HOST OVERWRITE E2E TEST\n";

    std::cout
        << "        REAL PHYSICAL DEVICE TEST\n";

    printSeparator();

    std::cout
        << "\nIMPORTANT:\n"
        << "This test can permanently destroy data on the selected device.\n"
        << "Use ONLY a disposable test device.\n"
        << "DO NOT select your Windows/system disk.\n";

    // ============================================================
    // STEP 1 - DEVICE DISCOVERY
    // ============================================================

    printSeparator();

    std::cout
        << "STEP 1 - DEVICE DISCOVERY\n";

    std::cout
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
            << "No physical storage devices were detected.\n";

        return 1;
    }

    std::cout
        << "\nDetected "
        << devices.size()
        << " physical storage device(s).\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
        printDevice(devices[i], i + 1);

    // ============================================================
    // TARGET SELECTION
    // ============================================================

    printSeparator();

    std::cout
        << "TARGET SELECTION\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "\nSelect the disposable test device.\n"
        << "Enter device number: ";

    const std::string selectedInput = readLine();

    std::size_t selectedIndex = 0;

    if (!parseDeviceIndex(
            selectedInput,
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

    std::cout
        << "\nSelected target:\n";

    printDevice(
        selectedDevice,
        selectedIndex + 1);

    // ============================================================
    // HARD SYSTEM-DISK GUARD
    // ============================================================

    if (selectedDevice.isSystemDisk())
    {
        printSeparator();

        std::cout
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "CRITICAL SAFETY BLOCK\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "The selected device is the Windows system disk.\n"
            << "Host overwrite will NOT be executed.\n";

        return 2;
    }

    if (selectedDevice.getDeviceId().empty())
    {
        std::cout
            << "\n[FAIL]\n"
            << "Target device ID is empty.\n";

        return 1;
    }

    if (selectedDevice.getSerialNumber().empty())
    {
        std::cout
            << "\n[FAIL]\n"
            << "Target serial number is empty.\n"
            << "Destructive test requires target identity.\n";

        return 1;
    }

    if (selectedDevice.getCapacityBytes() == 0)
    {
        std::cout
            << "\n[FAIL]\n"
            << "Target capacity is zero/unknown.\n";

        return 1;
    }

    // ============================================================
    // STEP 2 - SAFETY
    // ============================================================

    SafetyEngine safetyEngine;

    SafetyResult safetyResult;

    if (!runSafetyValidation(
            safetyEngine,
            selectedDevice,
            safetyResult))
    {
        return 2;
    }

    // ============================================================
    // STEP 3 - CAPABILITY + METHOD
    // ============================================================

    printSeparator();

    std::cout
        << "STEP 3 - CAPABILITY DETECTION & METHOD SELECTION\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "Detecting sanitization capability...\n";

    SanitizationCapability capability =
        detectSanitizationCapability(
            selectedDevice);

    std::cout
        << "\nCapability Summary\n"
        << "------------------\n"
        << "USB Device              : "
        << (capability.isUsbDevice ? "YES" : "NO") << '\n'
        << "Storage Property Query  : "
        << (capability.storagePropertyQueryAvailable
                ? "AVAILABLE"
                : "NOT AVAILABLE")
        << '\n'
        << "SCSI Path               : "
        << (capability.scsiPathAvailable
                ? "AVAILABLE"
                : "NOT AVAILABLE")
        << '\n'
        << "NVMe Identify           : "
        << (capability.nvmeIdentifyAvailable
                ? "AVAILABLE"
                : "NOT AVAILABLE")
        << '\n'
        << "ATA Identify            : "
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
        << "\nSelected Sanitization Method\n"
        << "----------------------------\n"
        << methodToString(method)
        << '\n';

    if (method != SanitizationMethod::HostOverwrite)
    {
        std::cout
            << "\n[TEST STOPPED]\n"
            << "This E2E test is specifically for the Host Overwrite path.\n"
            << "The selected device was not assigned Host Overwrite.\n"
            << "No destructive operation was started.\n";

        return 3;
    }

    std::cout
        << "\n[PASS]\n"
        << "Host Overwrite is the selected sanitization method.\n";

    // ============================================================
    // STEP 4 - EXPLICIT CONFIRMATION
    // ============================================================

    if (!confirmDestructiveOperation(
            selectedDevice))
    {
        return 4;
    }

    // ============================================================
    // STEP 5 - FINAL FRESH DISCOVERY
    // ============================================================

    printSeparator();

    std::cout
        << "STEP 5 - FINAL TARGET REVALIDATION\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "Performing fresh physical-device discovery...\n";

    std::vector<StorageDevice> freshDevices =
        discovery.discover();

    if (freshDevices.empty())
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Fresh device discovery returned no devices.\n"
            << "Target cannot be safely revalidated.\n";

        return 5;
    }

    if (!safetyEngine.validateTarget(
            freshDevices,
            selectedDevice))
    {
        std::cout
            << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "FINAL TARGET VALIDATION FAILED\n"
            << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
            << "The original target could not be matched safely.\n"
            << "No destructive operation will be executed.\n";

        return 5;
    }

    // Find the freshly discovered matching target.
    const StorageDevice* freshTarget = nullptr;

    for (const auto& device : freshDevices)
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
            << "Fresh target device was not found.\n";

        return 5;
    }

    selectedDevice = *freshTarget;

    if (selectedDevice.isSystemDisk())
    {
        std::cout
            << "\n[CRITICAL BLOCK]\n"
            << "Target is now detected as the system disk.\n"
            << "Sanitization aborted.\n";

        return 5;
    }

    SafetyResult finalSafetyResult =
        safetyEngine.evaluateWithResult(
            selectedDevice);

    if (!finalSafetyResult.isOverallSafe)
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Final safety evaluation failed.\n"
            << finalSafetyResult.summary
            << '\n';

        return 5;
    }

    std::cout
        << "\n[FINAL VALIDATION PASS]\n"
        << "Target identity and safety state remain valid.\n";

    // ============================================================
    // STEP 6 - FINAL WARNING
    // ============================================================

    printSeparator();

    std::cout
        << "STEP 6 - FINAL DESTRUCTIVE WARNING\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "\nTARGET LOCKED\n\n"
        << "Device ID : " << selectedDevice.getDeviceId() << '\n'
        << "Model     : " << selectedDevice.getModel() << '\n'
        << "Serial    : " << selectedDevice.getSerialNumber() << '\n'
        << "Capacity  : " << selectedDevice.getCapacityBytes()
        << " bytes\n"
        << "Method    : Host Overwrite\n"
        << "Pattern   : 0x00\n\n";

    std::cout
        << "The next operation will permanently overwrite\n"
        << "the entire physical device.\n\n";

    std::cout
        << "Enter START WIPE to begin: ";

    const std::string wipeConfirmation =
        readLine();

    if (wipeConfirmation != "START WIPE")
    {
        std::cout
            << "\n[ABORTED]\n"
            << "Final destructive command was not authorized.\n";

        return 6;
    }

    // ============================================================
    // STEP 7 - SANITIZATION
    // ============================================================

    printSeparator();

    std::cout
        << "STEP 7 - HOST OVERWRITE EXECUTION\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "\nSTARTING DESTRUCTIVE OPERATION.\n"
        << "DO NOT DISCONNECT THE DEVICE.\n"
        << "DO NOT POWER OFF THE COMPUTER.\n\n";

    std::cout
        << "Method  : Host Overwrite\n"
        << "Pattern : 0x00\n"
        << "Target  : "
        << selectedDevice.getDeviceId()
        << '\n';

    SecureWipe::SanitizationResult result =
        sanitizationEngine.sanitize(
            selectedDevice,
            finalSafetyResult);

    // ============================================================
    // STEP 8 - RESULT
    // ============================================================

    printSeparator();

    std::cout
        << "STEP 8 - FINAL SANITIZATION RESULT\n";

    std::cout
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

    std::cout << '\n';

    std::cout
        << "Message             : "
        << result.message
        << '\n';

    std::cout
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

    std::cout << '\n';

    std::cout
        << "Verification Samples : "
        << result.verificationSamples
        << '\n';

    std::cout
        << "Bytes Verified       : "
        << result.bytesVerified
        << '\n';

    std::cout
        << "Verification Message : "
        << result.verificationMessage
        << '\n';

    std::cout
        << "Duration             : "
        << result.operationDurationMs
        << " ms\n";

    // ============================================================
    // FINAL DECISION
    // ============================================================

    printSeparator();

    const bool finalPass =
        result.status ==
            SecureWipe::SanitizationStatus::COMPLETED &&
        result.verificationStatus ==
            SecureWipe::VerificationStatus::PASSED;

    if (finalPass)
    {
        std::cout
            << "              E2E TEST PASSED\n";
    }
    else
    {
        std::cout
            << "              E2E TEST FAILED\n";
    }

    printSeparator();

    if (finalPass)
    {
        std::cout
            << "\nHost Overwrite completed successfully.\n"
            << "Post-write verification passed.\n"
            << "The selected physical device was processed successfully.\n";
    }
    else
    {
        std::cout
            << "\nThe sanitization operation did not satisfy\n"
            << "the complete E2E success criteria.\n";
    }

    return finalPass ? 0 : 7;
}