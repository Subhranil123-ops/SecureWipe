#include "SanitizationEngine.h"
#include "NvmeSanitizer.h"
#include "HostOverwriteSanitizer.h"
#include "AtaSanitizer.h"

#include <Windows.h>
#include <iostream>
#include <chrono>

using SecureWipe::SanitizationErrorCode;
using SecureWipe::SanitizationResult;
using SecureWipe::SanitizationStatus;
using SecureWipe::VerificationStatus;

VerificationResult SanitizationEngine::performOverwrite(
    HANDLE deviceHandle,
    std::uint64_t totalBytes)
{
    VerificationResult result;

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.phase = "Handle validation";
        result.message =
            "Invalid device handle.";
        result.nativeErrorCode =
            ERROR_INVALID_HANDLE;
        return result;
    }

    if (totalBytes == 0)
    {
        result.phase = "Capacity validation";
        result.message =
            "Device capacity is zero.";
        result.nativeErrorCode =
            ERROR_INVALID_DATA;
        return result;
    }

    HostOverwriteSanitizer sanitizer;

    return sanitizer.sanitize(
        deviceHandle,
        totalBytes);
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
        if (capability.ataIdentifyAvailable)
        {
            std::cout
                << "ATA capability detected.\n";

            if (capability.atasanitizeSupported)
            {
                std::cout
                    << "ATA SANITIZE supported.\n";

                return SanitizationMethod::AtaSanitize;
            }

            std::cout
                << "ATA SANITIZE not supported.\n";
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

    // ------------------------------------------------------------
    // STEP 1 - SAFETY
    // ------------------------------------------------------------

    std::cout
        << "\n[1] Safety validation\n";

    if (!canSanitize(
            device,
            safetyResult))
    {
        result.status =
            SanitizationStatus::FAILED;

        result.error =
            SanitizationErrorCode::SAFETY_VALIDATION_FAILED;

        result.message =
            "Device failed sanitization safety checks.";

        result.errorMessage =
            result.message;

        return result;
    }

    std::cout
        << "Safety validation PASSED.\n";

    // ------------------------------------------------------------
    // STEP 2 - CAPABILITY
    // ------------------------------------------------------------

    std::cout
        << "\n[2] Detecting sanitization capability\n";

    const SanitizationCapability capability =
        detectSanitizationCapability(device);

    // ------------------------------------------------------------
    // STEP 3 - METHOD
    // ------------------------------------------------------------

    std::cout
        << "\n[3] Selecting sanitization method\n";

    const SanitizationMethod method =
        selectMethod(
            device,
            capability);

    result.method = method;

    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
        std::cout
            << "Selected method: NVMe Sanitize\n";
        break;

    case SanitizationMethod::AtaSanitize:
        std::cout
            << "Selected method: ATA Sanitize\n";
        break;

    case SanitizationMethod::HostOverwrite:
        std::cout
            << "Selected method: Host Overwrite\n";
        break;

    case SanitizationMethod::Unsupported:
        std::cout
            << "No supported sanitization method found.\n";
        break;
    }

    if (method ==
        SanitizationMethod::Unsupported)
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

        return result;
    }

    // ------------------------------------------------------------
    // STEP 4 - OPEN DEVICE
    // ------------------------------------------------------------

    std::cout
        << "\n[4] Opening sanitization target\n";

    HANDLE deviceHandle =
        CreateFileA(
            device.getDeviceId().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

    if (deviceHandle ==
        INVALID_HANDLE_VALUE)
    {
        const DWORD errorCode =
            GetLastError();

        result.status =
            SanitizationStatus::FAILED;

        result.error =
            SanitizationErrorCode::
                DEVICE_OPEN_FAILED;

        result.nativeErrorCode =
            static_cast<std::uint32_t>(
                errorCode);

        result.errorMessage =
            "Failed to open sanitization target. "
            "Windows error=" +
            std::to_string(errorCode);

        result.message =
            result.errorMessage;

        return result;
    }

    std::cout
        << "Device opened successfully.\n";

    const auto startTime =
        std::chrono::steady_clock::now();

    bool executionResult = false;

    VerificationResult verificationResult;

    // ------------------------------------------------------------
    // STEP 5 - EXECUTE SANITIZATION
    // ------------------------------------------------------------

    std::cout
        << "\n[5] Executing sanitization\n";

    switch (method)
    {
    case SanitizationMethod::NvmeSanitize:
    {
        std::cout
            << "Starting native NVMe sanitization.\n";

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
            << "Starting ATA sanitization.\n";

        AtaSanitizeMethod ataMethod;

        if (capability.atacryptoScrambleSupported)
        {
            ataMethod =
                AtaSanitizeMethod::CryptoScramble;
        }
        else if (capability.atablockEraseSupported)
        {
            ataMethod =
                AtaSanitizeMethod::BlockErase;
        }
        else if (capability.ataoverwriteSupported)
        {
            ataMethod =
                AtaSanitizeMethod::Overwrite;
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

        break;
    }

    case SanitizationMethod::HostOverwrite:
    {
        std::cout
            << "Starting host overwrite.\n";

        verificationResult =
            performOverwrite(
                deviceHandle,
                device.getCapacityBytes());

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

    // ------------------------------------------------------------
    // VERIFICATION RESULT MAPPING
    // ------------------------------------------------------------

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

    result.nativeErrorCode =
        verificationResult.nativeErrorCode;

    // ------------------------------------------------------------
    // DECIDE EXECUTION RESULT
    // ------------------------------------------------------------

    if (!verificationResult.performed)
    {
        result.error =
            SanitizationErrorCode::
                SANITIZATION_EXECUTION_FAILED;

        result.message =
            verificationResult.message;

        result.errorMessage =
            result.message;

        executionResult = false;
    }
    else if (!verificationResult.passed)
    {
        result.error =
            SanitizationErrorCode::
                VERIFICATION_FAILED;

        result.message =
            verificationResult.message;

        result.errorMessage =
            result.message;

        executionResult = false;
    }
    else
    {
        executionResult = true;
    }

    // ------------------------------------------------------------
    // STEP 6 - DURATION
    // ------------------------------------------------------------

    const auto endTime =
        std::chrono::steady_clock::now();

    result.operationDurationMs =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::milliseconds>(
                endTime - startTime)
                .count());

    // ------------------------------------------------------------
    // STEP 7 - CLOSE
    // ------------------------------------------------------------

    CloseHandle(deviceHandle);

    // ------------------------------------------------------------
    // STEP 8 - FINAL RESULT
    // ------------------------------------------------------------

    if (executionResult)
    {
        result.status =
            SanitizationStatus::COMPLETED;

        result.bytesProcessed =
            device.getCapacityBytes();

        result.message =
            "Sanitization completed successfully.";

        result.error =
            SanitizationErrorCode::NONE;
    }
    else
    {
        result.status =
            SanitizationStatus::FAILED;

        if (result.error ==
            SanitizationErrorCode::NONE)
        {
            result.error =
                SanitizationErrorCode::
                    SANITIZATION_EXECUTION_FAILED;
        }

        if (result.message.empty())
        {
            result.message =
                "Sanitization execution failed.";
        }

        result.errorMessage =
            result.message;
    }

    std::cout
        << "\n========================================\n"
        << " Sanitization Result\n"
        << "========================================\n";

    std::cout
        << "Result      : "
        << (executionResult
                ? "SUCCESS"
                : "FAILED")
        << '\n';

    std::cout
        << "Status      : "
        << (
            result.status ==
                SanitizationStatus::COMPLETED
                ? "COMPLETED"
                : "FAILED")
        << '\n';

    std::cout
        << "Bytes       : "
        << result.bytesProcessed
        << '\n';

    std::cout
        << "Verification: "
        << (
            result.verificationStatus ==
                VerificationStatus::PASSED
                ? "PASSED"
                : result.verificationStatus ==
                    VerificationStatus::FAILED
                    ? "FAILED"
                    : "NOT PERFORMED")
        << '\n';

    std::cout
        << "Samples     : "
        << result.verificationSamples
        << '\n';

    std::cout
        << "Verified    : "
        << result.bytesVerified
        << " bytes\n";

    std::cout
        << "Native Error: "
        << result.nativeErrorCode
        << '\n';

    std::cout
        << "Message     : "
        << result.message
        << '\n';

    std::cout
        << "Verification Message: "
        << result.verificationMessage
        << '\n';

    std::cout
        << "Duration    : "
        << result.operationDurationMs
        << " ms\n";

    return result;
}