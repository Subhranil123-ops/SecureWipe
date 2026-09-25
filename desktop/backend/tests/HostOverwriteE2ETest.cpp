#include <Windows.h>

#include <algorithm>
#include <cctype>
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
#include "AuditChainVerifier.h"
#include "CertificateVerifier.h"

using namespace SecureWipe;

namespace
{

// ============================================================
// CONSOLE STYLE
// ============================================================

namespace Console
{
    constexpr const char* RESET =
        "\x1b[0m";

    constexpr const char* CYAN =
        "\x1b[96m";

    constexpr const char* GREEN =
        "\x1b[92m";

    constexpr const char* YELLOW =
        "\x1b[93m";

    constexpr const char* RED =
        "\x1b[91m";

    constexpr const char* DIM =
        "\x1b[90m";

    constexpr const char* BOLD_CYAN =
        "\x1b[1;96m";

    constexpr const char* BOLD_GREEN =
        "\x1b[1;92m";

    constexpr const char* BOLD_RED =
        "\x1b[1;91m";

    constexpr const char* BOLD_YELLOW =
        "\x1b[1;93m";

    void enable()
    {
        HANDLE output =
            GetStdHandle(
                STD_OUTPUT_HANDLE);

        if (
            output == INVALID_HANDLE_VALUE ||
            output == nullptr)
        {
            return;
        }

        DWORD mode = 0;

        if (
            !GetConsoleMode(
                output,
                &mode))
        {
            return;
        }

        mode |=
            ENABLE_VIRTUAL_TERMINAL_PROCESSING;

        SetConsoleMode(
            output,
            mode);
    }

    void title(
        const std::string& text)
    {
        std::cout
            << "\n"
            << BOLD_CYAN
            << "+==================================================================+\n"
            << "| "
            << text;

        if (
            text.size() < 64)
        {
            std::cout
                << std::string(
                       64 - text.size(),
                       ' ');
        }

        std::cout
            << "|\n"
            << "+==================================================================+"
            << RESET
            << "\n";
    }

    void pass(
        const std::string& text)
    {
        std::cout
            << BOLD_GREEN
            << "  [PASS] "
            << RESET
            << text
            << '\n';
    }

    void fail(
        const std::string& text)
    {
        std::cout
            << BOLD_RED
            << "  [FAIL] "
            << RESET
            << text
            << '\n';
    }

    void stop(
        const std::string& text)
    {
        std::cout
            << BOLD_YELLOW
            << "  [STOP] "
            << RESET
            << text
            << '\n';
    }

    void warning(
        const std::string& text)
    {
        std::cout
            << BOLD_YELLOW
            << "  [WARN] "
            << RESET
            << text
            << '\n';
    }

    void info(
        const std::string& text)
    {
        std::cout
            << CYAN
            << text
            << RESET;
    }
}

// ============================================================
// INPUT
// ============================================================

std::string readLine()
{
    std::string value;

    std::getline(
        std::cin,
        value);

    return value;
}

// ============================================================
// SEPARATOR
// ============================================================

void separator()
{
    std::cout
        << "\n"
        << Console::DIM
        << "------------------------------------------------------------------"
        << Console::RESET
        << "\n";
}

// ============================================================
// STRING HELPERS
// ============================================================

std::string trim(
    std::string value)
{
    while (
        !value.empty() &&
        std::isspace(
            static_cast<unsigned char>(
                value.front())))
    {
        value.erase(
            value.begin());
    }

    while (
        !value.empty() &&
        std::isspace(
            static_cast<unsigned char>(
                value.back())))
    {
        value.pop_back();
    }

    return value;
}

std::string toUpper(
    std::string value)
{
    for (
        char& character :
        value)
    {
        character =
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(
                        character)));
    }

    return value;
}

// ============================================================
// ENUM DISPLAY HELPERS
// ============================================================

std::string methodToString(
    SanitizationMethod method)
{
    switch (
        method)
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

std::string sanitizationStatusToString(
    SanitizationStatus status)
{
    switch (
        status)
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

std::string verificationStatusToString(
    VerificationStatus status)
{
    switch (
        status)
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

// ============================================================
// WINDOWS ACTOR
// ============================================================

bool getCurrentWindowsUser(
    std::string& user)
{
    char buffer[256]{};

    DWORD size =
        static_cast<DWORD>(
            sizeof(buffer));

    if (
        !GetUserNameA(
            buffer,
            &size))
    {
        return false;
    }

    if (
        size > 0)
    {
        --size;
    }

    user.assign(
        buffer,
        size);

    return !user.empty();
}

// ============================================================
// DEVICE SELECTION
// ============================================================

bool parseIndex(
    const std::string& input,
    std::size_t count,
    std::size_t& index)
{
    try
    {
        std::size_t consumed = 0;

        const unsigned long long value =
            std::stoull(
                input,
                &consumed);

        if (
            consumed != input.size() ||
            value == 0 ||
            value > count)
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

// ============================================================
// DEVICE DISPLAY
// ============================================================

void printDevice(
    const StorageDevice& device,
    std::size_t number)
{
    std::cout
        << "\n"
        << Console::CYAN
        << "  [" << number << "]"
        << Console::RESET
        << "\n"

        << "      Device ID   : "
        << device.getDeviceId()
        << '\n'

        << "      Model       : "
        << device.getModel()
        << '\n'

        << "      Serial      : "
        << device.getSerialNumber()
        << '\n'

        << "      Interface   : "
        << device.getInterfaceType()
        << '\n'

        << "      Capacity    : "
        << device.getCapacityBytes()
        << " bytes\n"

        << "      System Disk : ";

    if (
        device.isSystemDisk())
    {
        std::cout
            << Console::BOLD_RED
            << "YES - BLOCKED"
            << Console::RESET;
    }
    else
    {
        std::cout
            << Console::GREEN
            << "NO"
            << Console::RESET;
    }

    std::cout
        << '\n'

        << "      Removable   : "
        << (
            device.isRemovable()
                ? "YES"
                : "NO"
        )
        << '\n';
}

// ============================================================
// DESTRUCTIVE CONFIRMATION
// ============================================================

bool confirmTarget(
    const StorageDevice& device)
{
    separator();

    std::cout
        << Console::BOLD_YELLOW
        << "FINAL DESTRUCTIVE AUTHORIZATION"
        << Console::RESET
        << "\n"

        << "------------------------------------------------------------\n"

        << "\n"
        << "This operation permanently overwrites the selected device.\n"
        << "Use ONLY a disposable/sacrificial device.\n\n"

        << "Device ID : "
        << device.getDeviceId()
        << '\n'

        << "Model     : "
        << device.getModel()
        << '\n'

        << "Serial    : "
        << device.getSerialNumber()
        << '\n'

        << "Capacity  : "
        << device.getCapacityBytes()
        << " bytes\n"

        << "Method    : Host Overwrite\n\n"

        << "Enter EXACT serial number: ";

    if (
        trim(
            readLine()) !=
        device.getSerialNumber())
    {
        Console::stop(
            "Serial number mismatch. Operation cancelled.");

        return false;
    }

    std::cout
        << "\nType EXACTLY:\n"
        << Console::BOLD_YELLOW
        << "START HOST OVERWRITE"
        << Console::RESET
        << "\n"
        << "Confirmation: ";

    if (
        toUpper(
            trim(
                readLine())) !=
        "START HOST OVERWRITE")
    {
        Console::stop(
            "Final destructive authorization failed.");

        return false;
    }

    Console::pass(
        "Destructive authorization accepted.");

    return true;
}

// ============================================================
// AUDIT FILE CONTENT CHECK
// ============================================================

bool containsText(
    const std::filesystem::path& file,
    const std::string& text)
{
    std::ifstream input(
        file,
        std::ios::in |
        std::ios::binary);

    if (!input)
        return false;

    std::string line;

    while (
        std::getline(
            input,
            line))
    {
        if (
            line.find(text) !=
            std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// ============================================================
// AUDIT EVENT PRESENCE CHECK
// ============================================================

bool validateAuditTrail(
    const SanitizationPipelineResult& result)
{
    separator();

    std::cout
        << Console::CYAN
        << "AUDIT EVENT PERSISTENCE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    const std::filesystem::path auditPath(
        result.auditLogPath);

    if (
        !std::filesystem::exists(
            auditPath))
    {
        Console::fail(
            "Audit log does not exist: " +
            auditPath.string());

        return false;
    }

    Console::pass(
        "Audit log exists.");

    const std::vector<std::string>
        requiredEvents = {
            "PIPELINE_STARTED",
            "SAFETY_CHECK_COMPLETED",
            "TARGET_VALIDATED",
            "SANITIZATION_STARTED",
            "SANITIZATION_COMPLETED",
            "VERIFICATION_COMPLETED",
            "CERTIFICATE_GENERATED",
            "CERTIFICATE_PERSISTED",
            "PIPELINE_COMPLETED"
        };

    bool passed = true;

    for (
        const auto& event :
        requiredEvents)
    {
        if (
            containsText(
                auditPath,
                "\"eventType\":\"" +
                    event +
                    "\""))
        {
            Console::pass(
                event);
        }
        else
        {
            Console::fail(
                "Missing audit event: " +
                event);

            passed = false;
        }
    }

    return passed;
}

// ============================================================
// CERTIFICATE EVIDENCE CHECK
// ============================================================

bool validateCertificate(
    const SanitizationPipelineResult& result,
    const StorageDevice& target,
    const std::string& expectedRequestId,
    const std::string& expectedWorkstationId)
{
    separator();

    std::cout
        << Console::CYAN
        << "CERTIFICATE EVIDENCE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    bool passed = true;

    const auto& certificate =
        result.certificate;

    const auto& sanitization =
        result.sanitization;

    if (
        certificate.certificateId.empty())
    {
        Console::fail(
            "Certificate ID is empty.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate ID generated.");
    }

    if (
        certificate.operationId.empty() ||
        certificate.operationId !=
            sanitization.operationId)
    {
        Console::fail(
            "Certificate operation ID mismatch.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Operation ID matches sanitization result.");
    }

    if (
        certificate.requestId.empty())
    {
        Console::fail(
            "Certificate request ID is empty.");

        passed = false;
    }
    else if (
        certificate.requestId !=
        expectedRequestId)
    {
        Console::fail(
            "Certificate request ID does not match the real web request.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Real request ID preserved.");
    }

    if (
        certificate.workstationId.empty())
    {
        Console::fail(
            "Certificate workstation ID is empty.");

        passed = false;
    }
    else if (
        certificate.workstationId !=
        expectedWorkstationId)
    {
        Console::fail(
            "Certificate workstation ID does not match the assigned workstation.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Real workstation ID preserved.");
    }

    if (
        certificate.deviceId !=
        target.getDeviceId())
    {
        Console::fail(
            "Certificate device ID mismatch.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Device ID matches.");
    }

    if (
        certificate.serialNumber !=
        target.getSerialNumber())
    {
        Console::fail(
            "Certificate serial number mismatch.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Serial number matches.");
    }

    if (
        certificate.method !=
        SanitizationMethod::HostOverwrite)
    {
        Console::fail(
            "Certificate method is not Host Overwrite.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate method is Host Overwrite.");
    }

    if (
        certificate.status !=
        SanitizationStatus::COMPLETED)
    {
        Console::fail(
            "Certificate status is not COMPLETED.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate status is COMPLETED.");
    }

    if (
        !certificate.verificationPerformed ||
        certificate.verificationStatus !=
            VerificationStatus::PASSED ||
        !certificate.verificationPassed)
    {
        Console::fail(
            "Certificate verification evidence is incomplete.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Verification evidence is valid.");
    }

    if (
        certificate.bytesVerified !=
            sanitization.bytesVerified ||
        certificate.verificationSamples !=
            sanitization.verificationSamples)
    {
        Console::fail(
            "Certificate verification counters do not match.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Verification counters match.");
    }

    if (
        certificate.certificateHash.empty() ||
        certificate.certificateHash.size() !=
            64)
    {
        Console::fail(
            "Certificate SHA-256 hash is missing or invalid.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate SHA-256 hash is present.");
    }

    if (
        !certificate.isValid())
    {
        Console::fail(
            "Certificate isValid() returned FALSE.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate isValid() returned TRUE.");
    }

    const std::filesystem::path certificatePath(
        result.certificatePath);

    if (
        !result.certificatePersisted ||
        result.certificatePath.empty() ||
        !std::filesystem::exists(
            certificatePath))
    {
        Console::fail(
            "Persisted certificate file is missing.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Persisted certificate file exists.");
    }

    return passed;
}

// ============================================================
// REAL SYSTEM-DISK SAFETY TEST
// ============================================================

bool runRealSystemDiskRejectionTest(
    const StorageDevice& systemDisk,
    const std::string& actorId)
{
    separator();

    std::cout
        << Console::CYAN
        << "CASE 1 - SYSTEM DISK PROTECTION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "\nThis case uses the ACTUAL Windows system disk.\n"
        << "No destructive write must occur.\n\n";

    printDevice(
        systemDisk,
        1);

    SanitizationPipeline pipeline;

    const SanitizationPipelineResult result =
        pipeline.execute(
            systemDisk,
            {},
            actorId);

    std::cout
        << "\nPipeline status   : "
        << sanitizationStatusToString(
               result.sanitization.status)
        << '\n'

        << "Pipeline message  : "
        << result.pipelineMessage
        << '\n'

        << "Bytes processed   : "
        << result.sanitization.bytesProcessed
        << '\n';

    bool passed = true;

    if (
        result.sanitization.status ==
        SanitizationStatus::COMPLETED)
    {
        Console::fail(
            "System disk was reported as sanitized.");

        passed = false;
    }
    else
    {
        Console::pass(
            "System disk sanitization was not completed.");
    }

    if (
        result.sanitization.bytesProcessed !=
        0)
    {
        Console::fail(
            "Bytes were processed on the system disk.");

        passed = false;
    }
    else
    {
        Console::pass(
            "No device bytes were processed.");
    }

    if (
        !result.auditTrailPersisted)
    {
        Console::fail(
            "System-disk safety rejection was not fully audited.");

        passed = false;
    }
    else
    {
        Console::pass(
            "System-disk safety rejection was audited.");
    }

    return passed;
}

// ============================================================
// MAIN
// ============================================================

} // namespace

int main()
{
        Console::enable();

    Console::title(
        "FORENWIPE  |  REAL DEVICE SANITIZATION");

    Console::info(
        "SECUREWIPE HOST OVERWRITE REAL E2E TEST\n");

    Console::info(
        "Production Path: Discover -> Safety -> Host Overwrite -> Verify -> Certify -> Audit\n");

    separator();

    std::cout
        << Console::BOLD_YELLOW
        << "WARNING: THIS IS A REAL DESTRUCTIVE TEST."
        << Console::RESET
        << "\n\n"

        << "Use ONLY a disposable/sacrificial physical storage device.\n"
        << "NEVER select the Windows system disk.\n";

    // =========================================================
    // ACTOR
    // =========================================================

    std::string actorId;

    if (
        !getCurrentWindowsUser(
            actorId))
    {
        Console::fail(
            "Could not determine the Windows actor.");

        return 1;
    }

    std::cout
        << "\nActor/User: "
        << actorId
        << '\n';

    // =========================================================
    // REAL WEB REQUEST ID
    // =========================================================

    separator();

    std::cout
        << Console::CYAN
        << "WEB JOB BINDING"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n"

        << "\nEnter REAL sanitization request ID: ";

    const std::string requestId =
        trim(
            readLine());

    if (
        requestId.empty())
    {
        Console::stop(
            "Request ID is required.");

        return 1;
    }

    // =========================================================
    // REAL WORKSTATION ID
    // =========================================================

    std::cout
        << "Enter REAL assigned workstation ID: ";

    const std::string workstationId =
        trim(
            readLine());

    if (
        workstationId.empty())
    {
        Console::stop(
            "Workstation ID is required.");

        return 1;
    }

    std::cout
        << "\n"
        << "Request ID     : "
        << requestId
        << '\n'

        << "Workstation ID : "
        << workstationId
        << '\n'

        << "Actor          : "
        << actorId
        << '\n';

    // =========================================================
    // STEP 1 - REAL DEVICE DISCOVERY
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 1 - REAL DEVICE DISCOVERY"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    WindowsStorageDiscovery discovery;

    std::vector<StorageDevice> devices;

    try
    {
        devices =
            discovery.discover();
    }
    catch (
        const std::exception& exception)
    {
        Console::fail(
            "Device discovery exception: " +
            std::string(
                exception.what()));

        return 1;
    }
    catch (...)
    {
        Console::fail(
            "Unknown exception during device discovery.");

        return 1;
    }

    if (
        devices.empty())
    {
        Console::fail(
            "No physical storage devices detected.");

        return 1;
    }

    Console::pass(
        "Detected " +
        std::to_string(
            devices.size()) +
        " physical device(s).");

    for (
        std::size_t i = 0;
        i < devices.size();
        ++i)
    {
        printDevice(
            devices[i],
            i + 1);
    }

    // =========================================================
    // LOCATE SYSTEM DISK
    // =========================================================

    const auto systemDiskIt =
        std::find_if(
            devices.begin(),
            devices.end(),
            [](const StorageDevice& device)
            {
                return device.isSystemDisk();
            });

    if (
        systemDiskIt ==
        devices.end())
    {
        Console::fail(
            "Windows system disk could not be identified.");

        return 2;
    }

    const StorageDevice systemDisk =
        *systemDiskIt;

    // =========================================================
    // CASE 1 - SYSTEM DISK REJECTION
    // =========================================================

    if (
        !runRealSystemDiskRejectionTest(
            systemDisk,
            actorId))
    {
        Console::fail(
            "System-disk safety test failed.");

        return 3;
    }

    Console::pass(
        "System-disk protection test passed.");

    // =========================================================
    // STEP 2 - SACRIFICIAL DEVICE SELECTION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 2 - SELECT SACRIFICIAL DEVICE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n"

        << "\nSelect ONLY the disposable physical device.\n"
        << "DO NOT select the system disk.\n\n"

        << "Device number: ";

    const std::string deviceInput =
        trim(
            readLine());

    std::size_t selectedIndex = 0;

    if (
        !parseIndex(
            deviceInput,
            devices.size(),
            selectedIndex))
    {
        Console::fail(
            "Invalid device selection.");

        return 4;
    }

    StorageDevice selectedDevice =
        devices[selectedIndex];

    printDevice(
        selectedDevice,
        selectedIndex + 1);

    // =========================================================
    // HARD SAFETY BLOCKS
    // =========================================================

    if (
        selectedDevice.isSystemDisk())
    {
        Console::stop(
            "Selected target is the Windows system disk. Destructive execution is blocked.");

        return 5;
    }

    if (
        selectedDevice.getDeviceId().empty())
    {
        Console::fail(
            "Real physical device ID is empty.");

        return 5;
    }

    if (
        selectedDevice.getSerialNumber().empty())
    {
        Console::fail(
            "Real physical serial number is empty.");

        return 5;
    }

    if (
        selectedDevice.getCapacityBytes() == 0)
    {
        Console::fail(
            "Real device capacity is zero.");

        return 5;
    }

    Console::pass(
        "Target has complete physical identity and capacity information.");

    // =========================================================
    // STEP 3 - PRE-FLIGHT SAFETY
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 3 - PRE-FLIGHT SAFETY"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    SafetyEngine safetyEngine;

    safetyEngine.setExpectedTarget(
        selectedDevice);

    const SafetyResult safetyResult =
        safetyEngine.evaluateWithResult(
            selectedDevice);

    std::cout
        << "\nDecision : "
        << safetyResult.decision
        << "\nSummary  : "
        << safetyResult.summary
        << "\n\n";

    for (
        const auto& check :
        safetyResult.checks)
    {
        if (
            check.passed)
        {
            Console::pass(
                check.checkName +
                " - " +
                check.message);
        }
        else
        {
            Console::fail(
                check.checkName +
                " - " +
                check.message);
        }
    }

    if (
        !safetyResult.isOverallSafe)
    {
        Console::stop(
            "Pre-flight safety rejected the target.");

        return 6;
    }

    Console::pass(
        "Pre-flight safety validation passed.");

    // =========================================================
    // STEP 4 - METHOD SELECTION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 4 - SANITIZATION METHOD"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    const SanitizationCapability capability =
        detectSanitizationCapability(
            selectedDevice);

    SanitizationEngine sanitizationEngine;

    const SanitizationMethod method =
        sanitizationEngine.selectMethod(
            selectedDevice,
            capability);

    std::cout
        << "Selected method : "
        << methodToString(
               method)
        << '\n';

    if (
        method !=
        SanitizationMethod::HostOverwrite)
    {
        Console::stop(
            "This executable validates the Host Overwrite path only. No destructive operation was started.");

        return 7;
    }

    Console::pass(
        "Host Overwrite selected.");

    // =========================================================
    // STEP 5 - FINAL TARGET REVALIDATION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 5 - FINAL TARGET REVALIDATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::vector<StorageDevice> freshDevices;

    try
    {
        freshDevices =
            discovery.discover();
    }
    catch (
        const std::exception& exception)
    {
        Console::fail(
            "Final device discovery failed: " +
            std::string(
                exception.what()));

        return 8;
    }
    catch (...)
    {
        Console::fail(
            "Unknown exception during final device discovery.");

        return 8;
    }

    StorageDevice freshTarget =
        selectedDevice;

    bool found = false;

    for (
        const auto& device :
        freshDevices)
    {
        if (
            device.getDeviceId() ==
            selectedDevice.getDeviceId())
        {
            freshTarget =
                device;

            found =
                true;

            break;
        }
    }

    if (!found)
    {
        Console::stop(
            "Selected target disappeared before destructive execution.");

        return 8;
    }

    if (
        freshTarget.isSystemDisk())
    {
        Console::stop(
            "Target is now identified as the Windows system disk.");

        return 8;
    }

    if (
        freshTarget.getSerialNumber() !=
            selectedDevice.getSerialNumber() ||
        freshTarget.getCapacityBytes() !=
            selectedDevice.getCapacityBytes())
    {
        Console::stop(
            "Target physical identity changed before execution.");

        return 8;
    }

    SafetyEngine finalSafetyEngine;

    finalSafetyEngine.setExpectedTarget(
        freshTarget);

    const SafetyResult finalSafetyResult =
        finalSafetyEngine.evaluateWithResult(
            freshTarget);

    if (
        !finalSafetyResult.isOverallSafe)
    {
        Console::stop(
            "Final safety validation failed: " +
            finalSafetyResult.summary);

        return 8;
    }

    selectedDevice =
        freshTarget;

    Console::pass(
        "Final physical identity confirmed.");

    Console::pass(
        "Final safety validation passed.");

    // =========================================================
    // STEP 6 - FINAL DESTRUCTIVE AUTHORIZATION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_YELLOW
        << "STEP 6 - FINAL DESTRUCTIVE AUTHORIZATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (
        !confirmTarget(
            selectedDevice))
    {
        return 9;
    }

    // =========================================================
    // STEP 7 - REAL HOST OVERWRITE PIPELINE
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 7 - REAL HOST OVERWRITE PIPELINE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    Console::info(
        "\nStarting native sanitization pipeline...\n");

    std::cout
        << Console::BOLD_YELLOW
        << "\n  DO NOT disconnect the target device.\n"
        << "  DO NOT power off the machine.\n"
        << "  DO NOT interrupt the operation.\n"
        << Console::RESET
        << '\n';

    SanitizationPipeline pipeline;

    const SanitizationPipelineResult result =
        pipeline.execute(
            selectedDevice,
            requestId,
            actorId,
            workstationId,
            selectedDevice.getSerialNumber());

    // =========================================================
    // STEP 8 - PIPELINE RESULT
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 8 - SANITIZATION RESULT"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Status             : "
        << sanitizationStatusToString(
               result.sanitization.status)
        << '\n'

        << "Method             : "
        << methodToString(
               result.sanitization.method)
        << '\n'

        << "Operation ID       : "
        << result.sanitization.operationId
        << '\n'

        << "Device ID          : "
        << result.sanitization.deviceId
        << '\n'

        << "Serial             : "
        << result.sanitization.serialNumber
        << '\n'

        << "Bytes Processed    : "
        << result.sanitization.bytesProcessed
        << '\n'

        << "Verification       : "
        << verificationStatusToString(
               result.sanitization.verificationStatus)
        << '\n'

        << "Verification Done  : "
        << (
            result.sanitization.verificationPerformed
                ? "YES"
                : "NO"
        )
        << '\n'

        << "Samples            : "
        << result.sanitization.verificationSamples
        << '\n'

        << "Bytes Verified     : "
        << result.sanitization.bytesVerified
        << '\n'

        << "Verification Msg   : "
        << result.sanitization.verificationMessage
        << '\n'

        << "Error              : "
        << result.sanitization.errorMessage
        << '\n'

        << "Pipeline Message   : "
        << result.pipelineMessage
        << '\n';

    // =========================================================
    // STEP 9 - NEVER CERTIFY A FAILED OPERATION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 9 - SUCCESS GATE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (
        !result.sanitization.isSuccess())
    {
        Console::fail(
            "Sanitization did not reach a verified-success state.");

        std::cout
            << "\nAudit Log: "
            << result.auditLogPath
            << '\n';

        return 10;
    }

    Console::pass(
        "Sanitization completed successfully.");

    /*
     * SanitizationResult does not contain a
     * verificationPassed member.
     *
     * isSuccess() already requires:
     *
     * status == COMPLETED
     * verificationStatus == PASSED
     */
    if (
        result.sanitization.verificationStatus !=
            VerificationStatus::PASSED ||
        !result.sanitization.verificationPerformed)
    {
        Console::fail(
            "Post-write verification evidence is not valid.");

        return 10;
    }

    Console::pass(
        "Post-write verification passed.");

    // =========================================================
    // STEP 10 - CERTIFICATE GENERATION / PERSISTENCE
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 10 - CERTIFICATE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (
        !result.certificateGenerated)
    {
        Console::fail(
            "Certificate generation failed.");

        return 11;
    }

    Console::pass(
        "Certificate generated.");

    if (
        !result.certificatePersisted ||
        result.certificatePath.empty() ||
        !std::filesystem::exists(
            result.certificatePath))
    {
        Console::fail(
            "Certificate JSON persistence failed.");

        return 11;
    }

    Console::pass(
        "Certificate JSON persisted.");

    std::cout
        << "\nCertificate ID:"
        << "\n  "
        << result.certificate.certificateId

        << "\n\nCertificate File:"
        << "\n  "
        << result.certificatePath

        << "\n\nCertificate SHA-256:"
        << "\n  "
        << result.certificate.certificateHash
        << '\n';
            // =========================================================
    // STEP 11 - NATIVE CERTIFICATE SHA-256 VERIFICATION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 11 - CERTIFICATE SHA-256 VERIFICATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    CertificateVerifier certificateVerifier;

    const CertificateVerificationResult
        certificateVerification =
            certificateVerifier.verify(
                result.certificatePath);

    if (
        certificateVerification.valid)
    {
        Console::pass(
            "Certificate structure verification passed.");

        Console::pass(
            "Certificate SHA-256 verification passed.");
    }
    else
    {
        Console::fail(
            certificateVerification.message);

        std::cout
            << "\nStored Hash:"
            << "\n  "
            << certificateVerification.storedHash

            << "\n\nCalculated Hash:"
            << "\n  "
            << certificateVerification.calculatedHash
            << '\n';

        return 12;
    }

    std::cout
        << "\nStored SHA-256:"
        << "\n  "
        << certificateVerification.storedHash

        << "\nCalculated SHA-256:"
        << "\n  "
        << certificateVerification.calculatedHash
        << '\n';

    // =========================================================
    // STEP 12 - AUDIT FILE PERSISTENCE
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 12 - AUDIT TRAIL"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (
        !result.auditTrailPersisted)
    {
        Console::fail(
            "Audit trail persistence is incomplete.");

        return 13;
    }

    if (
        result.auditLogPath.empty() ||
        !std::filesystem::exists(
            result.auditLogPath))
    {
        Console::fail(
            "Audit JSONL file does not exist.");

        return 13;
    }

    Console::pass(
        "Audit JSONL file persisted.");

    std::cout
        << "\nAudit Log:"
        << "\n  "
        << result.auditLogPath
        << '\n';

    if (
        validateAuditTrail(
            result))
    {
        Console::pass(
            "Required audit events are present.");
    }
    else
    {
        Console::fail(
            "Audit event persistence validation failed.");

        return 13;
    }

    // =========================================================
    // STEP 13 - NATIVE AUDIT HASH-CHAIN VERIFICATION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 13 - AUDIT SHA-256 HASH CHAIN"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    AuditChainVerifier auditVerifier;

    const AuditChainVerificationResult
        auditVerification =
            auditVerifier.verify(
                result.auditLogPath);

    if (
        !auditVerification.valid)
    {
        Console::fail(
            auditVerification.message);

        std::cout
            << "\nFailed Event Index:"
            << " "
            << auditVerification.failedEventIndex

            << "\nExpected Hash:"
            << "\n  "
            << auditVerification.expectedHash

            << "\nActual Hash:"
            << "\n  "
            << auditVerification.actualHash
            << '\n';

        return 14;
    }

    Console::pass(
        "Complete audit hash chain verified.");

    std::cout
        << "\nAudit Events:"
        << " "
        << auditVerification.eventCount

        << "\nVerified Events:"
        << " "
        << auditVerification.verifiedEventCount

        << "\nFirst Event Previous Hash:"
        << "\n  "
        << auditVerification.firstEventPreviousHash
        << '\n';

    // =========================================================
    // STEP 14 - FINAL EVIDENCE SUMMARY
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_GREEN
        << "+==================================================================+\n"
        << "|                    REAL E2E TEST PASSED                         |\n"
        << "+==================================================================+"
        << Console::RESET
        << "\n\n";

    std::cout
        << Console::GREEN
        << "  Physical Device       : PASS\n"
        << "  Safety Validation     : PASS\n"
        << "  Host Overwrite        : PASS\n"
        << "  Post-write Verify     : PASS\n"
        << "  Sanitization Result   : PASS\n"
        << "  Certificate Generated : PASS\n"
        << "  Certificate Persisted : PASS\n"
        << "  Certificate SHA-256   : PASS\n"
        << "  Audit JSONL Persisted : PASS\n"
        << "  Audit Hash Chain      : PASS\n"
        << Console::RESET
        << '\n';

    separator();

    std::cout
        << Console::CYAN
        << "REQUEST / JOB EVIDENCE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n"

        << "Request ID:"
        << "\n  "
        << requestId

        << "\n\nWorkstation ID:"
        << "\n  "
        << workstationId

        << "\n\nActor:"
        << "\n  "
        << actorId

        << "\n\nOperation ID:"
        << "\n  "
        << result.sanitization.operationId

        << "\n\nCertificate ID:"
        << "\n  "
        << result.certificate.certificateId

        << "\n\nCertificate File:"
        << "\n  "
        << result.certificatePath

        << "\n\nAudit Log:"
        << "\n  "
        << result.auditLogPath

        << "\n\nOperation Log:"
        << "\n  "
        << result.operationLogPath

        << "\n\nCertificate SHA-256:"
        << "\n  "
        << result.certificate.certificateHash
        << '\n';

    separator();

    std::cout
        << Console::BOLD_GREEN
        << "READY FOR WEB EVIDENCE UPLOAD"
        << Console::RESET
        << "\n";

    std::cout
        << "\nThe next step is to submit the REAL certificate JSON\n"
        << "and the REAL audit JSONL to the SecureWipe backend.\n";

    separator();

    return 0;
}