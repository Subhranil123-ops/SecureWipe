#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

#include "SanitizationEngine.h"
#include "SafetyEngine.h"
#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"
#include "SanitizationResult.h"

using SecureWipe::SanitizationResult;
using SecureWipe::SanitizationStatus;
using SecureWipe::VerificationStatus;
using SecureWipe::SanitizationErrorCode;

void printDevice(const StorageDevice& device, std::size_t index)
{
    std::cout << "\n[" << index << "]\n";
    std::cout << "Device ID   : " << device.getDeviceId() << '\n';
    std::cout << "Model       : " << device.getModel() << '\n';
    std::cout << "Serial      : " << device.getSerialNumber() << '\n';
    std::cout << "Capacity    : " << device.getCapacityBytes() << " bytes\n";
    std::cout << "Interface   : " << device.getInterfaceType() << '\n';
    std::cout << "System Disk : " << (device.isSystemDisk() ? "YES" : "NO") << '\n';
    std::cout << "Removable   : " << (device.isRemovable() ? "YES" : "NO") << '\n';
}

const char* getSanitizationStatusName(SanitizationStatus status)
{
    switch (status)
    {
    case SanitizationStatus::NOT_STARTED:
        return "NOT_STARTED";

    case SanitizationStatus::IN_PROGRESS:
        return "IN_PROGRESS";

    case SanitizationStatus::COMPLETED:
        return "COMPLETED";

    case SanitizationStatus::FAILED:
        return "FAILED";

    case SanitizationStatus::ABORTED:
        return "ABORTED";
    }

    return "UNKNOWN";
}

const char* getVerificationStatusName(VerificationStatus status)
{
    switch (status)
    {
    case VerificationStatus::NOT_PERFORMED:
        return "NOT_PERFORMED";

    case VerificationStatus::IN_PROGRESS:
        return "IN_PROGRESS";

    case VerificationStatus::PASSED:
        return "PASSED";

    case VerificationStatus::FAILED:
        return "FAILED";
    }

    return "UNKNOWN";
}

const char* getErrorCodeName(SanitizationErrorCode error)
{
    switch (error)
    {
    case SanitizationErrorCode::NONE:
        return "NONE";

    case SanitizationErrorCode::SAFETY_VALIDATION_FAILED:
        return "SAFETY_VALIDATION_FAILED";

    case SanitizationErrorCode::MISSING_DEVICE_ID:
        return "MISSING_DEVICE_ID";

    case SanitizationErrorCode::UNKNOWN_DEVICE_CAPACITY:
        return "UNKNOWN_DEVICE_CAPACITY";

    case SanitizationErrorCode::DEVICE_OPEN_FAILED:
        return "DEVICE_OPEN_FAILED";

    case SanitizationErrorCode::UNSUPPORTED_SANITIZATION_METHOD:
        return "UNSUPPORTED_SANITIZATION_METHOD";

    case SanitizationErrorCode::NVME_ALGORITHM_UNAVAILABLE:
        return "NVME_ALGORITHM_UNAVAILABLE";

    case SanitizationErrorCode::ATA_ALGORITHM_UNAVAILABLE:
        return "ATA_ALGORITHM_UNAVAILABLE";

    case SanitizationErrorCode::SANITIZATION_EXECUTION_FAILED:
        return "SANITIZATION_EXECUTION_FAILED";

    case SanitizationErrorCode::VERIFICATION_FAILED:
        return "VERIFICATION_FAILED";
    }

    return "UNKNOWN";
}

void printSanitizationResult(const SanitizationResult& result)
{
    std::cout << "\n============================================================\n";
    std::cout << "              SANITIZATION ENGINE RESULT\n";
    std::cout << "============================================================\n";

    std::cout << "Device ID          : " << result.deviceId << '\n';
    std::cout << "Model              : " << result.model << '\n';
    std::cout << "Serial Number      : " << result.serialNumber << '\n';
    std::cout << "Interface          : " << result.interfaceType << '\n';
    std::cout << "Capacity           : " << result.capacityBytes << " bytes\n";

    std::cout << "\nStatus             : "
              << getSanitizationStatusName(result.status) << '\n';

    std::cout << "Method             : ";

    switch (result.method)
    {
    case SanitizationMethod::NvmeSanitize:
        std::cout << "NVMe Sanitize";
        break;

    case SanitizationMethod::AtaSanitize:
        std::cout << "ATA Sanitize";
        break;

    case SanitizationMethod::HostOverwrite:
        std::cout << "Host Overwrite";
        break;

    case SanitizationMethod::Unsupported:
        std::cout << "Unsupported";
        break;
    }

    std::cout << '\n';

    std::cout << "Bytes Processed    : "
              << result.bytesProcessed << '\n';

    std::cout << "Duration           : "
              << result.operationDurationMs << " ms\n";

    std::cout << "\nVerification       : "
              << getVerificationStatusName(result.verificationStatus)
              << '\n';

    std::cout << "Verification Done  : "
              << (result.verificationPerformed ? "YES" : "NO")
              << '\n';

    std::cout << "Verification Samples: "
              << result.verificationSamples << '\n';

    std::cout << "Bytes Verified     : "
              << result.bytesVerified << '\n';

    if (!result.verificationMessage.empty())
    {
        std::cout << "Verification Msg   : "
                  << result.verificationMessage << '\n';
    }

    std::cout << "\nError Code         : "
              << getErrorCodeName(result.error) << '\n';

    std::cout << "Native Error Code  : "
              << result.nativeErrorCode << '\n';

    std::cout << "Message            : "
              << result.message << '\n';

    if (!result.errorMessage.empty())
    {
        std::cout << "Error Message      : "
                  << result.errorMessage << '\n';
    }

    std::cout << "\n------------------------------------------------------------\n";

    if (result.isSuccess())
    {
        std::cout << "FINAL RESULT       : PASS\n";
        std::cout << "Sanitization       : COMPLETED\n";
        std::cout << "Verification       : PASSED\n";
    }
    else
    {
        std::cout << "FINAL RESULT       : FAIL\n";
        std::cout << "Sanitization       : NOT SUCCESSFUL\n";
    }

    std::cout << "============================================================\n";
}

bool confirmDestructiveOperation(const StorageDevice& device)
{
    std::cout << "\n============================================================\n";
    std::cout << "              DESTRUCTIVE OPERATION WARNING\n";
    std::cout << "============================================================\n";

    std::cout << "\nThis operation will permanently destroy all data\n";
    std::cout << "on the selected storage device.\n\n";

    printDevice(device, 0);

    std::cout << "\nTo continue, type EXACTLY:\n";
    std::cout << "ERASE " << device.getSerialNumber() << '\n';

    std::cout << "\nConfirmation: ";

    std::string confirmation;
    std::getline(std::cin >> std::ws, confirmation);

    const std::string expected =
        "ERASE " + device.getSerialNumber();

    if (confirmation != expected)
    {
        std::cout << "\n[ABORTED] Confirmation did not match.\n";
        return false;
    }

    return true;
}

int main()
{
    std::cout << "============================================================\n";
    std::cout << "          SecureWipe Sanitization Engine Test\n";
    std::cout << "============================================================\n";

    // ========================================================
    // STEP 1: DEVICE DISCOVERY
    // ========================================================

    std::cout << "\n[STEP 1] Storage Device Discovery\n";
    std::cout << "------------------------------------------------------------\n";

    WindowsStorageDiscovery discovery;

    const std::vector<StorageDevice> devices =
        discovery.discover();

    if (devices.empty())
    {
        std::cout << "[FAIL] No storage devices detected.\n";
        return 1;
    }

    std::cout << "Detected devices: "
              << devices.size() << '\n';

    for (std::size_t i = 0; i < devices.size(); ++i)
    {
        printDevice(devices[i], i);
    }

    // ========================================================
    // STEP 2: TARGET SELECTION
    // ========================================================

    std::cout << "\n[STEP 2] Target Selection\n";
    std::cout << "------------------------------------------------------------\n";

    std::cout << "Enter device index: ";

    std::size_t selectedIndex = 0;
    std::cin >> selectedIndex;

    if (selectedIndex >= devices.size())
    {
        std::cout << "[FAIL] Invalid device index.\n";
        return 1;
    }

    const StorageDevice selectedDevice =
        devices[selectedIndex];

    std::cout << "\nSelected Target:\n";
    printDevice(selectedDevice, selectedIndex);

    // ========================================================
    // STEP 3: BASIC SAFETY CHECK
    // ========================================================

    std::cout << "\n[STEP 3] Basic Target Safety Validation\n";
    std::cout << "------------------------------------------------------------\n";

    if (selectedDevice.isSystemDisk())
    {
        std::cout << "[BLOCKED] Selected device is the system disk.\n";
        std::cout << "Destructive sanitization is not allowed.\n";
        return 1;
    }

    if (selectedDevice.getDeviceId().empty())
    {
        std::cout << "[FAIL] Device ID is missing.\n";
        return 1;
    }

    if (selectedDevice.getCapacityBytes() == 0)
    {
        std::cout << "[FAIL] Device capacity is unknown.\n";
        return 1;
    }

    std::cout << "[PASS] Basic target validation passed.\n";

    // ========================================================
    // STEP 4: SAFETY ENGINE
    // ========================================================

    std::cout << "\n[STEP 4] Safety Engine Evaluation\n";
    std::cout << "------------------------------------------------------------\n";

    SafetyEngine safetyEngine;

    safetyEngine.setExpectedTarget(selectedDevice);

    const SafetyResult safetyResult =
        safetyEngine.evaluateWithResult(selectedDevice);

    std::cout << "\nSafety Decision: "
              << (safetyResult.isOverallSafe ? "SAFE" : "BLOCKED")
              << '\n';

    std::cout << "Decision: "
              << safetyResult.decision << '\n';

    std::cout << "Summary : "
              << safetyResult.summary << '\n';

    if (!safetyResult.isOverallSafe)
    {
        std::cout << "\n[BLOCKED] Safety Engine rejected the target.\n";

        for (const auto& check : safetyResult.checks)
        {
            std::cout << check.checkName
                      << " : "
                      << (check.passed ? "PASS" : "FAIL")
                      << '\n';

            std::cout << "  "
                      << check.message
                      << '\n';
        }

        return 1;
    }

    std::cout << "[PASS] Safety Engine approved the target.\n";

    // ========================================================
    // STEP 5: FRESH DISCOVERY AND IDENTITY VALIDATION
    // ========================================================

    std::cout << "\n[STEP 5] Fresh Target Identity Validation\n";
    std::cout << "------------------------------------------------------------\n";

    const std::vector<StorageDevice> freshDevices =
        discovery.discover();

    if (freshDevices.empty())
    {
        std::cout << "[FAIL] Fresh device discovery returned no devices.\n";
        return 1;
    }

    StorageDevice validatedTarget =
        selectedDevice;

    const bool validationResult =
        safetyEngine.validateTarget(
            freshDevices,
            validatedTarget);

    if (!validationResult)
    {
        std::cout << "[BLOCKED] Target identity validation failed.\n";
        std::cout << "The target may have disappeared or changed.\n";
        return 1;
    }

    std::cout << "[PASS] Target identity validated.\n";

    printDevice(validatedTarget, selectedIndex);

    // ========================================================
    // STEP 6: FINAL DESTRUCTIVE CONFIRMATION
    // ========================================================

    std::cout << "\n[STEP 6] Final Destructive Confirmation\n";

    if (!confirmDestructiveOperation(validatedTarget))
    {
        std::cout << "\nSanitization test aborted by user.\n";
        return 0;
    }

    // ========================================================
    // STEP 7: ACTUAL SANITIZATION ENGINE
    // ========================================================

    std::cout << "\n[STEP 7] Executing Sanitization Engine\n";
    std::cout << "============================================================\n";

    SanitizationEngine sanitizationEngine;

    /*
     * IMPORTANT:
     *
     * sanitize() returns SanitizationResult.
     * It does NOT return bool.
     */
    const SanitizationResult result =
        sanitizationEngine.sanitize(
            validatedTarget,
            safetyResult);

    // ========================================================
    // STEP 8: RESULT
    // ========================================================

    std::cout << "\n[STEP 8] Final Sanitization Report\n";

    printSanitizationResult(result);

    return result.isSuccess() ? 0 : 1;
}