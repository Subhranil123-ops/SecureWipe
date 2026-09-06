#include "SanitizationEngine.h"

#include "NvmeSanitizer.h"
#include "HostOverwriteSanitizer.h"
#include "AtaSanitizer.h"

#include <Windows.h>
#include <chrono>
#include <iostream>
#include <string>

using SecureWipe::SanitizationErrorCode;
using SecureWipe::SanitizationResult;
using SecureWipe::SanitizationStatus;
using SecureWipe::VerificationStatus;

namespace
{
const char* methodName(
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

    default:
        return "Unsupported";
    }
}
}

SanitizationMethod SanitizationEngine::selectMethod(
    const StorageDevice& device,
    const SanitizationCapability& capability) const
{
    if (device.getInterfaceType() == "NVMe")
    {
        if (capability.nvmeIdentifyAvailable &&
            capability.nativeSanitizeSupported ==
                NativeSanitizeSupport::SUPPORTED)
        {
            return SanitizationMethod::NvmeSanitize;
        }

        return SanitizationMethod::Unsupported;
    }

    if (device.getInterfaceType() == "SATA")
    {
        if (!capability.ataIdentifyAvailable)
        {
            return SanitizationMethod::Unsupported;
        }

        if (capability.atasanitizeSupported)
        {
            return SanitizationMethod::AtaSanitize;
        }

        return SanitizationMethod::HostOverwrite;
    }

    if (capability.isUsbDevice &&
        capability.scsiPathAvailable)
    {
        return SanitizationMethod::HostOverwrite;
    }

    return SanitizationMethod::Unsupported;
}

bool SanitizationEngine::canSanitize(
    const StorageDevice& device,
    const SafetyResult& safetyResult)
{
    if (!safetyResult.isOverallSafe)
    {
        std::cout
            << "Safety Engine rejected the device.\n";

        return false;
    }

    if (device.getDeviceId().empty())
    {
        std::cout
            << "Device ID is missing.\n";

        return false;
    }

    if (device.getCapacityBytes() == 0)
    {
        std::cout
            << "Device capacity is unknown.\n";

        return false;
    }

    return true;
}

SanitizationResult SanitizationEngine::sanitize(
    const StorageDevice& device,
    const SafetyResult& safetyResult)
{
    SanitizationResult result;

    result.operationId =
        std::to_string(
            static_cast<unsigned long long>(
                std::chrono::duration_cast<
                    std::chrono::microseconds>(
                    std::chrono::steady_clock::now()
                        .time_since_epoch())
                    .count()));

    result.deviceId =
        device.getDeviceId();

    result.model =
        device.getModel();

    result.serialNumber =
        device.getSerialNumber();

    result.interfaceType =
        device.getInterfaceType();

    result.capacityBytes =
        device.getCapacityBytes();

    result.status =
        SanitizationStatus::IN_PROGRESS;

    result.verificationStatus =
        VerificationStatus::NOT_PERFORMED;

    std::cout
        << "\n========================================\n"
        << " Sanitization Engine\n"
        << "========================================\n";

    // --------------------------------------------------
    // STEP 1: SAFETY
    // --------------------------------------------------

    std::cout
        << "\n[1] Safety validation\n";

    if (!canSanitize(
            device,
            safetyResult))
    {
        result.status =
            SanitizationStatus::FAILED;

        result.error =
            SanitizationErrorCode::
                SAFETY_VALIDATION_FAILED;

        result.message =
            "Device failed sanitization safety checks.";

        result.errorMessage =
            result.message;

        return result;
    }

    std::cout
        << "Safety validation PASSED.\n";

    // --------------------------------------------------
    // STEP 2: CAPABILITY
    // --------------------------------------------------

    std::cout
        << "\n[2] Detecting sanitization capability\n";

    const SanitizationCapability capability =
        detectSanitizationCapability(
            device);

    // --------------------------------------------------
    // STEP 3: METHOD
    // --------------------------------------------------

    std::cout
        << "\n[3] Selecting sanitization method\n";

    const SanitizationMethod method =
        selectMethod(
            device,
            capability);

    result.method =
        method;

    std::cout
        << "Selected method: "
        << methodName(method)
        << '\n';

    if (method ==
        SanitizationMethod::Unsupported)
    {
        result.status =
            SanitizationStatus::FAILED;

        result.error =
            SanitizationErrorCode::
                UNSUPPORTED_SANITIZATION_METHOD;

        result.message =
            "No supported sanitization method found for this target.";

        result.errorMessage =
            result.message;

        return result;
    }

    // --------------------------------------------------
    // STEP 4: OPEN PHYSICAL DEVICE
    // --------------------------------------------------

    std::cout
        << "\n[4] Opening physical device\n";

    HANDLE deviceHandle =
        CreateFileA(
            device.getDeviceId().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ |
                FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

    if (deviceHandle ==
        INVALID_HANDLE_VALUE)
    {
        DWORD errorCode =
            GetLastError();

        result.status =
            SanitizationStatus::FAILED;

        result.error =
            SanitizationErrorCode::
                DEVICE_OPEN_FAILED;

        result.nativeErrorCode =
            static_cast<std::uint32_t>(
                errorCode);

        result.message =
            "Failed to open sanitization target. Windows error=" +
            std::to_string(errorCode) +
            ". Run as Administrator and ensure the device is not in use.";

        result.errorMessage =
            result.message;

        return result;
    }

    std::cout
        << "Physical device opened successfully.\n";

    const auto start =
        std::chrono::steady_clock::now();

    bool executionResult =
        false;

    VerificationResult verificationResult;

    // --------------------------------------------------
    // STEP 5: ACTUAL SANITIZATION
    // --------------------------------------------------

    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
    {
        std::cout
            << "\nStarting native NVMe sanitization.\n";

        NvmeSanitizeMethod nvmeMethod;

        if (capability.nvmeCryptoEraseSupported)
        {
            nvmeMethod =
                NvmeSanitizeMethod::CryptoErase;

            std::cout
                << "NVMe algorithm: Crypto Erase\n";
        }
        else if (capability.nvmeBlockEraseSupported)
        {
            nvmeMethod =
                NvmeSanitizeMethod::BlockErase;

            std::cout
                << "NVMe algorithm: Block Erase\n";
        }
        else if (capability.nvmeOverwriteSupported)
        {
            nvmeMethod =
                NvmeSanitizeMethod::Overwrite;

            std::cout
                << "NVMe algorithm: Overwrite\n";
        }
        else
        {
            result.status =
                SanitizationStatus::FAILED;

            result.error =
                SanitizationErrorCode::
                    NVME_ALGORITHM_UNAVAILABLE;

            result.message =
                "No supported NVMe sanitize algorithm found.";

            result.errorMessage =
                result.message;

            CloseHandle(deviceHandle);

            return result;
        }

        executionResult =
            executeNvmeSanitize(
                deviceHandle,
                nvmeMethod);

        break;
    }

    case SanitizationMethod::AtaSanitize:
    {
        std::cout
            << "\nStarting ATA sanitization.\n";

        AtaSanitizeMethod ataMethod;

        if (capability.atacryptoScrambleSupported)
        {
            ataMethod =
                AtaSanitizeMethod::CryptoScramble;

            std::cout
                << "ATA algorithm: Crypto Scramble EXT\n";
        }
        else if (capability.atablockEraseSupported)
        {
            ataMethod =
                AtaSanitizeMethod::BlockErase;

            std::cout
                << "ATA algorithm: Block Erase EXT\n";
        }
        else if (capability.ataoverwriteSupported)
        {
            ataMethod =
                AtaSanitizeMethod::Overwrite;

            std::cout
                << "ATA algorithm: Overwrite EXT\n";
        }
        else
        {
            result.status =
                SanitizationStatus::FAILED;

            result.error =
                SanitizationErrorCode::
                    ATA_ALGORITHM_UNAVAILABLE;

            result.message =
                "No supported ATA sanitize algorithm found.";

            result.errorMessage =
                result.message;

            CloseHandle(deviceHandle);

            return result;
        }

        verificationResult =
            executeAtaSanitize(
                deviceHandle,
                ataMethod);

        executionResult =
            verificationResult.sanitizationCompleted;

        break;
    }

    case SanitizationMethod::HostOverwrite:
    {
        std::cout
            << "\nStarting REAL HOST OVERWRITE.\n"
            << "Pattern: 0x00\n"
            << "Target size: "
            << device.getCapacityBytes()
            << " bytes\n";

        HostOverwriteSanitizer sanitizer;

        verificationResult =
            sanitizer.sanitize(
                deviceHandle,
                device.getCapacityBytes());

        executionResult =
            verificationResult.sanitizationCompleted;

        break;
    }

    case SanitizationMethod::Unsupported:
    {
        result.status =
            SanitizationStatus::FAILED;

        result.error =
            SanitizationErrorCode::
                UNSUPPORTED_SANITIZATION_METHOD;

        result.message =
            "No supported sanitization method found.";

        result.errorMessage =
            result.message;

        CloseHandle(deviceHandle);

        return result;
    }
    }

    // --------------------------------------------------
    // STEP 6: DURATION
    // --------------------------------------------------

    const auto end =
        std::chrono::steady_clock::now();

    result.operationDurationMs =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::milliseconds>(
                end - start)
                .count());

    // --------------------------------------------------
    // STEP 7: COPY VERIFICATION RESULT
    // --------------------------------------------------

    result.verificationPerformed =
        verificationResult.performed;

    result.verificationStatus =
        verificationResult.performed
            ? (verificationResult.passed
                ? VerificationStatus::PASSED
                : VerificationStatus::FAILED)
            : VerificationStatus::NOT_PERFORMED;

    result.bytesVerified =
        verificationResult.bytesVerified;

    result.verificationSamples =
        verificationResult.samples;

    result.verificationMessage =
        verificationResult.message;

    // --------------------------------------------------
    // STEP 8: CLOSE PHYSICAL DEVICE
    // --------------------------------------------------

    CloseHandle(deviceHandle);

    // --------------------------------------------------
    // STEP 9: FINAL RESULT
    // --------------------------------------------------

    if (!executionResult)
    {
        result.status =
            SanitizationStatus::FAILED;

        if (result.error ==
            SanitizationErrorCode::NONE)
        {
            if (method ==
                SanitizationMethod::HostOverwrite &&
                verificationResult.performed &&
                !verificationResult.passed)
            {
                result.error =
                    SanitizationErrorCode::
                        VERIFICATION_FAILED;
            }
            else
            {
                result.error =
                    SanitizationErrorCode::
                        SANITIZATION_EXECUTION_FAILED;
            }
        }

        if (!verificationResult.message.empty())
        {
            result.message =
                verificationResult.message;
        }
        else
        {
            result.message =
                "Sanitization execution failed.";
        }

        result.errorMessage =
            result.message;

        std::cout
            << "\nSanitization FAILED.\n"
            << result.message
            << '\n';

        return result;
    }

    result.status =
        SanitizationStatus::COMPLETED;

    result.bytesProcessed =
        device.getCapacityBytes();

    result.message =
        std::string(
            methodName(method)) +
        " execution completed successfully.";

    if (method ==
        SanitizationMethod::HostOverwrite)
    {
        if (verificationResult.passed)
        {
            result.message =
                "Host overwrite completed and post-write verification passed.";
        }
    }

    std::cout
        << "\nSanitization Engine Result: SUCCESS\n"
        << "Method: "
        << methodName(method)
        << '\n'
        << "Bytes processed: "
        << result.bytesProcessed
        << '\n'
        << "Verification: "
        << (result.verificationStatus ==
                VerificationStatus::PASSED
                ? "PASSED"
                : "NOT PERFORMED")
        << '\n'
        << "Duration: "
        << result.operationDurationMs
        << " ms\n";

    return result;
}