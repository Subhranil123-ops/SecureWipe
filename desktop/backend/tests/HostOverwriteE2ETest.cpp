#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"
#include "SanitizationCapability.h"
#include "SanitizationEngine.h"
#include "SanitizationMethod.h"
#include "SanitizationPipeline.h"
#include "SanitizationResult.h"
#include "SanitizationCertificate.h"
#include "SanitizationEvent.h"
#include "SafetyEngine.h"
#include "SafetyResult.h"

using namespace SecureWipe;

namespace
{
std::string readLine()
{
    std::string value;
    std::getline(std::cin, value);
    return value;
}

void separator()
{
    std::cout << "\n============================================================\n";
}

std::string methodToString(SanitizationMethod method)
{
    switch (method)
    {
    case SanitizationMethod::NvmeSanitize: return "NVMe Sanitize";
    case SanitizationMethod::AtaSanitize: return "ATA Sanitize";
    case SanitizationMethod::HostOverwrite: return "Host Overwrite";
    case SanitizationMethod::Unsupported: return "Unsupported";
    }
    return "Unknown";
}

std::string sanitizationStatusToString(SanitizationStatus status)
{
    switch (status)
    {
    case SanitizationStatus::NOT_STARTED: return "NOT_STARTED";
    case SanitizationStatus::IN_PROGRESS: return "IN_PROGRESS";
    case SanitizationStatus::COMPLETED: return "COMPLETED";
    case SanitizationStatus::FAILED: return "FAILED";
    case SanitizationStatus::ABORTED: return "ABORTED";
    }
    return "UNKNOWN";
}

std::string verificationStatusToString(VerificationStatus status)
{
    switch (status)
    {
    case VerificationStatus::NOT_PERFORMED: return "NOT_PERFORMED";
    case VerificationStatus::IN_PROGRESS: return "IN_PROGRESS";
    case VerificationStatus::PASSED: return "PASSED";
    case VerificationStatus::FAILED: return "FAILED";
    }
    return "UNKNOWN";
}

bool getCurrentWindowsUser(std::string& user)
{
    char buffer[256]{};
    DWORD size = static_cast<DWORD>(sizeof(buffer));

    if (!GetUserNameA(buffer, &size))
        return false;

    if (size > 0)
        --size;

    user.assign(buffer, size);
    return !user.empty();
}

bool parseIndex(const std::string& input, std::size_t count, std::size_t& index)
{
    try
    {
        std::size_t consumed = 0;
        const unsigned long long value = std::stoull(input, &consumed);

        if (consumed != input.size() || value == 0 || value > count)
            return false;

        index = static_cast<std::size_t>(value - 1);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void printDevice(const StorageDevice& device, std::size_t number)
{
    std::cout
        << "\n[" << number << "]\n"
        << "  Device ID   : " << device.getDeviceId() << '\n'
        << "  Model       : " << device.getModel() << '\n'
        << "  Serial      : " << device.getSerialNumber() << '\n'
        << "  Interface   : " << device.getInterfaceType() << '\n'
        << "  Capacity    : " << device.getCapacityBytes() << " bytes\n"
        << "  System Disk : " << (device.isSystemDisk() ? "YES - BLOCKED" : "NO") << '\n'
        << "  Removable   : " << (device.isRemovable() ? "YES" : "NO") << '\n';
}

bool confirmTarget(const StorageDevice& device)
{
    separator();
    std::cout
        << "FINAL DESTRUCTIVE AUTHORIZATION\n"
        << "------------------------------------------------------------\n"
        << "This will PERMANENTLY DESTROY ALL DATA on the selected device.\n\n"
        << "Device ID : " << device.getDeviceId() << '\n'
        << "Model     : " << device.getModel() << '\n'
        << "Serial    : " << device.getSerialNumber() << '\n'
        << "Interface : " << device.getInterfaceType() << '\n'
        << "Capacity  : " << device.getCapacityBytes() << " bytes\n"
        << "Method    : Host Overwrite\n"
        << "Pattern   : 0x00\n\n"
        << "Enter EXACT serial number: ";

    if (readLine() != device.getSerialNumber())
    {
        std::cout << "\n[ABORTED] Serial number mismatch.\n";
        return false;
    }

    std::cout << "Type EXACTLY: START HOST OVERWRITE\nConfirmation: ";

    if (readLine() != "START HOST OVERWRITE")
    {
        std::cout << "\n[ABORTED] Destructive authorization failed.\n";
        return false;
    }

    return true;
}

bool containsText(const std::filesystem::path& file, const std::string& text)
{
    std::ifstream input(file, std::ios::in | std::ios::binary);
    if (!input)
        return false;

    std::string line;
    while (std::getline(input, line))
    {
        if (line.find(text) != std::string::npos)
            return true;
    }

    return false;
}

bool validateAuditTrail(const SanitizationPipelineResult& result)
{
    separator();
    std::cout << "AUDIT TRAIL VALIDATION\n"
              << "------------------------------------------------------------\n";

    const std::filesystem::path auditPath(result.auditLogPath);

    if (!std::filesystem::exists(auditPath))
    {
        std::cout << "[FAIL] Audit log does not exist: " << auditPath << '\n';
        return false;
    }

    std::cout << "[PASS] Audit log exists.\n";

    const std::vector<std::string> requiredEvents = {
        "PIPELINE_STARTED",
        "SAFETY_CHECK_COMPLETED",
        "TARGET_VALIDATED",
        "METHOD_SELECTED",
        "SANITIZATION_STARTED",
        "SANITIZATION_COMPLETED",
        "VERIFICATION_COMPLETED",
        "CERTIFICATE_GENERATED",
        "CERTIFICATE_PERSISTED",
        "PIPELINE_COMPLETED"
    };

    bool passed = true;

    for (const auto& event : requiredEvents)
    {
        if (containsText(auditPath, "\"eventType\":\"" + event + "\""))
            std::cout << "[PASS] " << event << '\n';
        else
        {
            std::cout << "[FAIL] Missing audit event: " << event << '\n';
            passed = false;
        }
    }

    return passed;
}

bool validateCertificate(const SanitizationPipelineResult& result, const StorageDevice& target)
{
    separator();
    std::cout << "CERTIFICATE VALIDATION\n"
              << "------------------------------------------------------------\n";

    bool passed = true;

    const auto& certificate = result.certificate;
    const auto& sanitization = result.sanitization;

    if (certificate.certificateId.empty())
    {
        std::cout << "[FAIL] Certificate ID is empty.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Certificate ID generated.\n";

    if (certificate.operationId != sanitization.operationId || certificate.operationId.empty())
    {
        std::cout << "[FAIL] Certificate operation ID mismatch.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Operation ID matches.\n";

    if (certificate.requestId.empty())
    {
        std::cout << "[FAIL] Certificate request ID is empty.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Request ID preserved.\n";

    if (certificate.deviceId != target.getDeviceId())
    {
        std::cout << "[FAIL] Certificate device ID mismatch.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Device ID matches.\n";

    if (certificate.serialNumber != target.getSerialNumber())
    {
        std::cout << "[FAIL] Certificate serial number mismatch.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Serial number matches.\n";

    if (certificate.method != SanitizationMethod::HostOverwrite)
    {
        std::cout << "[FAIL] Certificate method is not Host Overwrite.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Certificate method is Host Overwrite.\n";

    if (certificate.status != SanitizationStatus::COMPLETED)
    {
        std::cout << "[FAIL] Certificate status is not COMPLETED.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Certificate status is COMPLETED.\n";

    if (!certificate.verificationPerformed ||
        certificate.verificationStatus != VerificationStatus::PASSED ||
        !certificate.verificationPassed)
    {
        std::cout << "[FAIL] Certificate verification evidence is incomplete.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Certificate verification evidence is valid.\n";

    if (certificate.bytesVerified != sanitization.bytesVerified ||
        certificate.verificationSamples != sanitization.verificationSamples)
    {
        std::cout << "[FAIL] Certificate verification counters do not match result.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Verification counters match.\n";

    if (certificate.certificateHash.empty() || certificate.certificateHash.size() != 64)
    {
        std::cout << "[FAIL] Certificate SHA-256 hash is missing/invalid.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Certificate SHA-256 hash is present.\n";

    if (!certificate.isValid())
    {
        std::cout << "[FAIL] Certificate isValid() returned FALSE.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Certificate isValid() returned TRUE.\n";

    const std::filesystem::path certificatePath(result.certificatePath);

    if (!result.certificatePersisted ||
        result.certificatePath.empty() ||
        !std::filesystem::exists(certificatePath))
    {
        std::cout << "[FAIL] Persisted certificate file is missing.\n";
        passed = false;
    }
    else
        std::cout << "[PASS] Persisted certificate file exists.\n";

    return passed;
}
}

int main()
{
    separator();
    std::cout
        << "SECUREWIPE HOST OVERWRITE REAL E2E TEST\n"
        << "Production Path: Discover -> Safety -> Host Overwrite -> Verify -> Certify -> Audit\n";
    separator();

    std::cout
        << "\nWARNING: THIS IS A REAL DESTRUCTIVE TEST.\n"
        << "Use ONLY a disposable/sacrificial physical storage device.\n"
        << "NEVER select the Windows system disk.\n\n";

    std::string actorId;
    if (!getCurrentWindowsUser(actorId))
    {
        std::cout << "[FAIL] Could not determine Windows user.\n";
        return 1;
    }

    std::cout << "Actor/User: " << actorId << '\n';

    std::cout << "Enter REAL sanitization request ID: ";
    const std::string requestId = readLine();

    if (requestId.empty())
    {
        std::cout << "[ABORTED] Request ID is required.\n";
        return 1;
    }

    // =========================================================
    // 1. REAL DEVICE DISCOVERY
    // =========================================================
    separator();
    std::cout << "STEP 1 - REAL DEVICE DISCOVERY\n"
              << "------------------------------------------------------------\n";

    WindowsStorageDiscovery discovery;
    const std::vector<StorageDevice> devices = discovery.discover();

    if (devices.empty())
    {
        std::cout << "[FAIL] No physical storage devices detected.\n";
        return 1;
    }

    std::cout << "Detected " << devices.size() << " physical device(s).\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
        printDevice(devices[i], i + 1);

    // =========================================================
    // 2. REAL TARGET SELECTION
    // =========================================================
    separator();
    std::cout << "STEP 2 - REAL TARGET SELECTION\n"
              << "------------------------------------------------------------\n";
    std::cout << "Enter the number of the disposable target device: ";

    std::size_t selectedIndex = 0;
    if (!parseIndex(readLine(), devices.size(), selectedIndex))
    {
        std::cout << "[FAIL] Invalid device selection.\n";
        return 1;
    }

    StorageDevice selectedDevice = devices[selectedIndex];
    printDevice(selectedDevice, selectedIndex + 1);

    if (selectedDevice.isSystemDisk())
    {
        std::cout << "[CRITICAL BLOCK] Selected target is the Windows system disk.\n";
        return 2;
    }

    if (selectedDevice.getDeviceId().empty() ||
        selectedDevice.getSerialNumber().empty() ||
        selectedDevice.getCapacityBytes() == 0)
    {
        std::cout << "[FAIL] Target has incomplete physical identity/capacity information.\n";
        return 1;
    }

    // =========================================================
    // 3. REAL PRE-FLIGHT SAFETY
    // =========================================================
    separator();
    std::cout << "STEP 3 - REAL PRE-FLIGHT SAFETY\n"
              << "------------------------------------------------------------\n";

    SafetyEngine safetyEngine;
    safetyEngine.setExpectedTarget(selectedDevice);
    const SafetyResult safetyResult =
        safetyEngine.evaluateWithResult(selectedDevice);

    std::cout << "Decision: " << safetyResult.decision << '\n'
              << "Summary : " << safetyResult.summary << '\n';

    for (const auto& check : safetyResult.checks)
    {
        std::cout
            << (check.passed ? "[PASS] " : "[FAIL] ")
            << check.checkName << " - " << check.message << '\n';
    }

    if (!safetyResult.isOverallSafe)
    {
        std::cout << "[BLOCKED] Pre-flight safety failed.\n";
        return 2;
    }

    // =========================================================
    // 4. REAL HOST OVERWRITE METHOD CHECK
    // =========================================================
    separator();
    std::cout << "STEP 4 - REAL METHOD SELECTION\n"
              << "------------------------------------------------------------\n";

    const SanitizationCapability capability =
        detectSanitizationCapability(selectedDevice);

    SanitizationEngine sanitizationEngine;
    const SanitizationMethod method =
        sanitizationEngine.selectMethod(selectedDevice, capability);

    std::cout << "Selected method: " << methodToString(method) << '\n';

    if (method != SanitizationMethod::HostOverwrite)
    {
        std::cout
            << "[STOPPED] This E2E executable is specifically for the Host Overwrite path.\n"
            << "No destructive operation was started.\n";
        return 3;
    }

    std::cout << "[PASS] Host Overwrite path selected.\n";

    // =========================================================
    // 5. FINAL REAL REDISCOVERY + IDENTITY CONFIRMATION
    // =========================================================
    separator();
    std::cout << "STEP 5 - FINAL TARGET REVALIDATION\n"
              << "------------------------------------------------------------\n";

    const std::vector<StorageDevice> freshDevices = discovery.discover();

    StorageDevice freshTarget = selectedDevice;
    bool found = false;

    for (const auto& device : freshDevices)
    {
        if (device.getDeviceId() == selectedDevice.getDeviceId())
        {
            freshTarget = device;
            found = true;
            break;
        }
    }

    if (!found)
    {
        std::cout << "[ABORTED] Target disappeared before execution.\n";
        return 5;
    }

    if (freshTarget.isSystemDisk())
    {
        std::cout << "[CRITICAL BLOCK] Target became the system disk.\n";
        return 5;
    }

    if (freshTarget.getSerialNumber() != selectedDevice.getSerialNumber() ||
        freshTarget.getCapacityBytes() != selectedDevice.getCapacityBytes())
    {
        std::cout << "[ABORTED] Target identity changed before execution.\n";
        return 5;
    }

    SafetyEngine finalSafetyEngine;
    finalSafetyEngine.setExpectedTarget(freshTarget);
    const SafetyResult finalSafetyResult =
        finalSafetyEngine.evaluateWithResult(freshTarget);

    if (!finalSafetyResult.isOverallSafe)
    {
        std::cout
            << "[ABORTED] Final safety evaluation failed: "
            << finalSafetyResult.summary << '\n';
        return 5;
    }

    selectedDevice = freshTarget;

    std::cout << "[PASS] Final target identity and safety state confirmed.\n";

    // =========================================================
    // 6. EXPLICIT DESTRUCTIVE AUTHORIZATION
    // =========================================================
    if (!confirmTarget(selectedDevice))
        return 6;

    // =========================================================
    // 7. PRODUCTION PIPELINE
    // =========================================================
    separator();
    std::cout << "STEP 7 - PRODUCTION SANITIZATION PIPELINE\n"
              << "------------------------------------------------------------\n"
              << "Starting real Host Overwrite pipeline...\n"
              << "DO NOT disconnect the target device.\n"
              << "DO NOT power off the machine.\n\n";

    SanitizationPipeline pipeline;
    const SanitizationPipelineResult result =
        pipeline.execute(
            selectedDevice,
            requestId,
            actorId);

    // =========================================================
    // 8. RESULT
    // =========================================================
    separator();
    std::cout << "STEP 8 - PIPELINE RESULT\n"
              << "------------------------------------------------------------\n"
              << "Status             : "
              << sanitizationStatusToString(result.sanitization.status) << '\n'
              << "Method             : "
              << methodToString(result.sanitization.method) << '\n'
              << "Operation ID       : "
              << result.sanitization.operationId << '\n'
              << "Device ID          : "
              << result.sanitization.deviceId << '\n'
              << "Serial             : "
              << result.sanitization.serialNumber << '\n'
              << "Bytes Processed    : "
              << result.sanitization.bytesProcessed << '\n'
              << "Verification       : "
              << verificationStatusToString(result.sanitization.verificationStatus) << '\n'
              << "Verification Done  : "
              << (result.sanitization.verificationPerformed ? "YES" : "NO") << '\n'
              << "Samples            : "
              << result.sanitization.verificationSamples << '\n'
              << "Bytes Verified     : "
              << result.sanitization.bytesVerified << '\n'
              << "Verification Msg   : "
              << result.sanitization.verificationMessage << '\n'
              << "Error              : "
              << result.sanitization.errorMessage << '\n'
              << "Pipeline Message   : "
              << result.pipelineMessage << '\n';

    if (!result.sanitization.isSuccess())
    {
        std::cout << "\n[FAIL] SanitizationResult is not a verified success.\n";
        std::cout << "Audit log: " << result.auditLogPath << '\n';
        return 7;
    }

    if (!result.certificateGenerated || !result.certificatePersisted)
    {
        std::cout << "\n[FAIL] Certificate generation/persistence failed.\n";
        return 8;
    }

    if (!validateCertificate(result, selectedDevice))
    {
        std::cout << "\n[FAIL] Certificate validation failed.\n";
        return 8;
    }

    if (!result.auditTrailPersisted || !validateAuditTrail(result))
    {
        std::cout << "\n[FAIL] Audit trail validation failed.\n";
        return 9;
    }

    // =========================================================
    // 9. FINAL PASS
    // =========================================================
    separator();
    std::cout
        << "REAL HOST OVERWRITE VERTICAL SLICE PASSED\n"
        << "------------------------------------------------------------\n"
        << "Physical Device      : PASS\n"
        << "Safety Validation    : PASS\n"
        << "Host Overwrite       : PASS\n"
        << "Post-write Verify    : PASS\n"
        << "SanitizationResult   : PASS\n"
        << "Certificate          : PASS\n"
        << "Certificate Persist  : PASS\n"
        << "Audit Trail          : PASS\n"
        << "Evidence Persistence : PASS\n\n"
        << "Request ID           : " << requestId << '\n'
        << "Actor                : " << actorId << '\n'
        << "Operation ID         : " << result.sanitization.operationId << '\n'
        << "Certificate ID       : " << result.certificate.certificateId << '\n'
        << "Certificate File     : " << result.certificatePath << '\n'
        << "Audit Log            : " << result.auditLogPath << '\n'
        << "Operation Log        : " << result.operationLogPath << '\n';

    separator();
    return 0;
}
