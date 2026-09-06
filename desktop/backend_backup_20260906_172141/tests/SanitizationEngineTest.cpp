#include <iostream>
#include <vector>
#include <string>

#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"
#include "SafetyEngine.h"
#include "SafetyResult.h"
#include "SanitizationEngine.h"
#include "SanitizationCapability.h"
#include "SanitizationMethod.h"

static const char* getMethodName(SanitizationMethod method)
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

    default:
        return "Unknown";
    }
}

static const char* getNativeSupportName(NativeSanitizeSupport support)
{
    switch (support)
    {
    case NativeSanitizeSupport::SUPPORTED:
        return "SUPPORTED";

    case NativeSanitizeSupport::NOT_SUPPORTED:
        return "NOT SUPPORTED";

    case NativeSanitizeSupport::UNKNOWN:
        return "UNKNOWN";

    default:
        return "UNKNOWN";
    }
}

static void printCapability(const SanitizationCapability& capability)
{
    std::cout << "\n========== Capability Result ==========\n";

    std::cout
        << "USB Device                  : "
        << (capability.isUsbDevice ? "YES" : "NO")
        << '\n';

    std::cout
        << "Storage Property Query      : "
        << (capability.storagePropertyQueryAvailable ? "AVAILABLE" : "NOT AVAILABLE")
        << '\n';

    std::cout
        << "SCSI Path                   : "
        << (capability.scsiPathAvailable ? "AVAILABLE" : "NOT AVAILABLE")
        << '\n';

    std::cout
        << "Native Sanitize             : "
        << getNativeSupportName(capability.nativeSanitizeSupported)
        << '\n';

    std::cout
        << "NVMe Identify               : "
        << (capability.nvmeIdentifyAvailable ? "AVAILABLE" : "NOT AVAILABLE")
        << '\n';

    std::cout
        << "NVMe Crypto Erase           : "
        << (capability.nvmeCryptoEraseSupported ? "SUPPORTED" : "NOT SUPPORTED")
        << '\n';

    std::cout
        << "NVMe Block Erase            : "
        << (capability.nvmeBlockEraseSupported ? "SUPPORTED" : "NOT SUPPORTED")
        << '\n';

    std::cout
        << "NVMe Overwrite              : "
        << (capability.nvmeOverwriteSupported ? "SUPPORTED" : "NOT SUPPORTED")
        << '\n';

    std::cout << "=======================================\n";
}

int main()
{
    std::cout
        << "========================================\n"
        << " SecureWipe Sanitization Engine Test\n"
        << "========================================\n";

    // ============================================================
    // STEP 1: DEVICE DISCOVERY
    // ============================================================

    WindowsStorageDiscovery discovery;

    std::vector<StorageDevice> devices =
        discovery.discover();

    if (devices.empty())
    {
        std::cout << "\nNo storage devices detected.\n";
        return 1;
    }

    std::cout
        << "\nDetected Devices: "
        << devices.size()
        << '\n';

    SafetyEngine safetyEngine;
    SanitizationEngine sanitizationEngine;

    // ============================================================
    // STEP 2: DEVICE INFORMATION + SAFETY + CAPABILITY + METHOD
    // ============================================================

    for (std::size_t i = 0; i < devices.size(); ++i)
    {
        const StorageDevice& device = devices[i];

        std::cout
            << "\n========================================\n"
            << " DEVICE "
            << i + 1
            << '\n'
            << "========================================\n";

        std::cout
            << "Device ID     : "
            << device.getDeviceId()
            << '\n';

        std::cout
            << "Model         : "
            << device.getModel()
            << '\n';

        std::cout
            << "Serial        : "
            << device.getSerialNumber()
            << '\n';

        std::cout
            << "Interface     : "
            << device.getInterfaceType()
            << '\n';

        std::cout
            << "Capacity      : "
            << device.getCapacityBytes()
            << " bytes\n";

        // --------------------------------------------------------
        // STEP 2.1: SAFETY CHECK
        // --------------------------------------------------------

        std::cout
            << "\n[1] Running Safety Engine...\n";

        safetyEngine.setExpectedTarget(device);

        SafetyResult safetyResult =
            safetyEngine.evaluateWithResult(device);

        std::cout
            << "Safety Decision : "
            << safetyResult.decision
            << '\n';

        std::cout
            << "Safety Summary  : "
            << safetyResult.summary
            << '\n';

        std::cout << "\nSafety Checks\n";
        std::cout << "-------------\n";

        for (const auto& check : safetyResult.checks)
        {
            std::cout
                << check.checkName
                << " : "
                << (check.passed ? "PASS" : "FAIL")
                << '\n';

            std::cout
                << "  "
                << check.message
                << '\n';
        }

        if (!safetyResult.isOverallSafe)
        {
            std::cout
                << "\n>>> SANITIZATION BLOCKED <<<\n"
                << "Safety Engine rejected this device.\n";

            continue;
        }

        std::cout
            << "\nSafety validation PASSED.\n";

        // --------------------------------------------------------
        // STEP 2.2: CAPABILITY DETECTION
        // --------------------------------------------------------

        std::cout
            << "\n[2] Detecting Sanitization Capability...\n";

        SanitizationCapability capability =
            detectSanitizationCapability(device);

        printCapability(capability);

        // --------------------------------------------------------
        // STEP 2.3: METHOD SELECTION
        // --------------------------------------------------------

        std::cout
            << "\n[3] Selecting Sanitization Method...\n";

        SanitizationMethod method =
            sanitizationEngine.selectMethod(
                device,
                capability);

        std::cout
            << "Selected Method : "
            << getMethodName(method)
            << '\n';

        if (method ==
            SanitizationMethod::Unsupported)
        {
            std::cout
                << "\n>>> SANITIZATION UNSUPPORTED <<<\n"
                << "No supported sanitization method was found.\n";

            continue;
        }

        std::cout
            << "\nDevice is eligible for the sanitization flow.\n";
    }

    // ============================================================
    // STEP 3: SELECT ONE DEVICE FOR ACTUAL SANITIZATION
    // ============================================================

    std::cout
        << "\n\n========================================\n"
        << " SELECT SANITIZATION TARGET\n"
        << "========================================\n";

    std::cout
        << "Enter device number to sanitize.\n"
        << "Enter 0 to exit without sanitizing.\n\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
    {
        std::cout
            << i + 1
            << ". "
            << devices[i].getDeviceId()
            << " | "
            << devices[i].getModel()
            << " | "
            << devices[i].getInterfaceType()
            << '\n';
    }

    std::cout
        << "\nSelection: ";

    int selectedDevice = 0;
    std::cin >> selectedDevice;

    if (selectedDevice <= 0 ||
        selectedDevice >
            static_cast<int>(devices.size()))
    {
        std::cout
            << "\nNo sanitization performed.\n";

        return 0;
    }

    const StorageDevice& target =
        devices[selectedDevice - 1];

    // ============================================================
    // STEP 4: DISPLAY FINAL TARGET
    // ============================================================

    std::cout
        << "\n========================================\n"
        << " FINAL TARGET CONFIRMATION\n"
        << "========================================\n";

    std::cout
        << "Device ID     : "
        << target.getDeviceId()
        << '\n';

    std::cout
        << "Model         : "
        << target.getModel()
        << '\n';

    std::cout
        << "Serial        : "
        << target.getSerialNumber()
        << '\n';

    std::cout
        << "Interface     : "
        << target.getInterfaceType()
        << '\n';

    std::cout
        << "Capacity      : "
        << target.getCapacityBytes()
        << " bytes\n";

    std::cout
        << "\nWARNING: Sanitization can permanently destroy data.\n"
        << "Make absolutely sure this is your disposable test device.\n"
        << "\nType YES to continue: ";

    std::string confirmation;
    std::cin >> confirmation;

    if (confirmation != "YES")
    {
        std::cout
            << "\nSanitization cancelled.\n";

        return 0;
    }

    // ============================================================
    // STEP 5: FINAL SAFETY VALIDATION
    // ============================================================

    std::cout
        << "\n[4] Final Safety Validation...\n";

    safetyEngine.setExpectedTarget(target);

    SafetyResult finalSafetyResult =
        safetyEngine.evaluateWithResult(target);

    if (!finalSafetyResult.isOverallSafe)
    {
        std::cout
            << "\n>>> SANITIZATION BLOCKED <<<\n"
            << "Final Safety Engine validation failed.\n";

        return 1;
    }

    std::cout
        << "Final Safety Validation : PASSED\n";

    // ============================================================
    // STEP 6: FINAL CAPABILITY CHECK
    // ============================================================

    std::cout
        << "\n[5] Final Capability Detection...\n";

    SanitizationCapability finalCapability =
        detectSanitizationCapability(target);

    printCapability(finalCapability);

    SanitizationMethod finalMethod =
        sanitizationEngine.selectMethod(
            target,
            finalCapability);

    std::cout
        << "\nFinal Sanitization Method : "
        << getMethodName(finalMethod)
        << '\n';

    if (finalMethod ==
        SanitizationMethod::Unsupported)
    {
        std::cout
            << "\n>>> SANITIZATION ABORTED <<<\n"
            << "No supported sanitization method is available.\n";

        return 1;
    }

    // ============================================================
    // STEP 7: ACTUAL SANITIZATION
    // ============================================================

    std::cout
        << "\n[6] Starting Sanitization Engine...\n";

    SecureWipe::SanitizationResult result =
        sanitizationEngine.sanitize(
            target,
            finalSafetyResult);

    // ============================================================
    // STEP 8: FINAL RESULT
    // ============================================================

    std::cout
        << "\n========================================\n"
        << " FINAL SANITIZATION RESULT\n"
        << "========================================\n";

    std::cout
        << "Device ID       : "
        << result.deviceId
        << '\n';

    std::cout
        << "Model           : "
        << result.model
        << '\n';

    std::cout
        << "Serial          : "
        << result.serialNumber
        << '\n';

    std::cout
        << "Interface       : "
        << result.interfaceType
        << '\n';

    std::cout
        << "Message         : "
        << result.message
        << '\n';

    std::cout
        << "Bytes Processed : "
        << result.bytesProcessed
        << '\n';

    std::cout
        << "Verification    : ";

    if (result.verificationStatus ==
        SecureWipe::VerificationStatus::PASSED)
    {
        std::cout << "PASSED\n";
    }
    else if (result.verificationStatus ==
             SecureWipe::VerificationStatus::FAILED)
    {
        std::cout << "FAILED\n";
    }
    else
    {
        std::cout << "NOT PERFORMED\n";
    }

    std::cout
        << "Verification Msg : "
        << result.verificationMessage
        << '\n';

    std::cout
        << "\n========================================\n"
        << " Test Completed\n"
        << "========================================\n";

    return 0;
}