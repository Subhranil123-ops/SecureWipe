#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#include "WindowsStorageDiscovery.h"
#include "StorageDevice.h"
#include "SanitizationPipeline.h"
#include "SanitizationMethod.h"
#include "SanitizationResult.h"
#include "SanitizationCertificate.h"

using namespace SecureWipe;

namespace
{
std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string toUpper(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::toupper(c));
        });

    return value;
}

void separator()
{
    std::cout
        << "\n============================================================\n";
}

void printDevice(
    const StorageDevice& device,
    std::size_t index)
{
    std::cout
        << "\n[" << index << "]\n"
        << "  Device ID   : " << device.getDeviceId() << '\n'
        << "  Model       : " << device.getModel() << '\n'
        << "  Serial      : " << device.getSerialNumber() << '\n'
        << "  Interface   : " << device.getInterfaceType() << '\n'
        << "  Capacity    : " << device.getCapacityBytes() << " bytes\n"
        << "  System Disk : "
        << (device.isSystemDisk() ? "YES" : "NO")
        << '\n';
}

std::string methodToString(SanitizationMethod method)
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

std::string statusToString(SanitizationStatus status)
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

std::string verificationToString(VerificationStatus status)
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

std::string getWindowsUserName()
{
    char buffer[256] = {};
    DWORD size = static_cast<DWORD>(sizeof(buffer));

    if (GetUserNameA(buffer, &size))
    {
        if (size > 0)
            return std::string(buffer, size - 1);
    }

    return "UNKNOWN_WINDOWS_USER";
}

bool parseIndex(
    const std::string& input,
    std::size_t count,
    std::size_t& index)
{
    try
    {
        const std::string value = trim(input);

        if (value.empty())
            return false;

        std::size_t consumed = 0;
        const unsigned long long parsed =
            std::stoull(value, &consumed);

        if (consumed != value.size())
            return false;

        if (parsed == 0 || parsed > count)
            return false;

        index =
            static_cast<std::size_t>(
                parsed - 1);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool fileExistsAndNonEmpty(
    const std::filesystem::path& path)
{
    std::error_code ec;

    if (!std::filesystem::exists(path, ec))
        return false;

    if (!std::filesystem::is_regular_file(path, ec))
        return false;

    const auto size =
        std::filesystem::file_size(path, ec);

    if (ec)
        return false;

    return size > 0;
}

std::size_t countAuditLines(
    const std::filesystem::path& path)
{
    std::ifstream input(path);

    if (!input)
        return 0;

    std::size_t count = 0;
    std::string line;

    while (std::getline(input, line))
    {
        if (!trim(line).empty())
            ++count;
    }

    return count;
}

bool readFile(
    const std::filesystem::path& path,
    std::string& content)
{
    std::ifstream input(
        path,
        std::ios::in |
        std::ios::binary);

    if (!input)
        return false;

    content.assign(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());

    return true;
}

bool validateAuditEvidence(
    const SanitizationPipelineResult& pipelineResult,
    const StorageDevice& device,
    const std::string& actorId)
{
    separator();

    std::cout
        << "AUDIT EVIDENCE VALIDATION\n"
        << "------------------------------------------------------------\n";

    bool passed = true;

    const std::filesystem::path auditPath =
        pipelineResult.auditLogPath;

    const std::filesystem::path operationLogPath =
        pipelineResult.operationLogPath;

    std::cout
        << "\nAudit log:\n"
        << auditPath.string()
        << "\n";

    if (!fileExistsAndNonEmpty(auditPath))
    {
        std::cout
            << "[FAIL] Audit log does not exist or is empty.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Audit log exists and contains data.\n";
    }

    std::cout
        << "\nOperation log:\n"
        << operationLogPath.string()
        << "\n";

    if (!fileExistsAndNonEmpty(operationLogPath))
    {
        std::cout
            << "[FAIL] Operation log does not exist or is empty.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Operation log exists and contains data.\n";
    }

    const std::size_t auditLines =
        countAuditLines(auditPath);

    std::cout
        << "\nAudit events recorded: "
        << auditLines
        << '\n';

    if (auditLines < 2)
    {
        std::cout
            << "[FAIL] Expected multiple pipeline audit events.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Multiple audit events were persisted.\n";
    }

    std::string auditContent;

    if (!readFile(auditPath, auditContent))
    {
        std::cout
            << "[FAIL] Unable to read audit log.\n";

        passed = false;
    }
    else
    {
        const bool hasDeviceId =
            !device.getDeviceId().empty() &&
            auditContent.find(
                device.getDeviceId()) !=
                std::string::npos;

        if (!hasDeviceId)
        {
            std::cout
                << "[FAIL] Real device ID was not found in audit evidence.\n";

            passed = false;
        }
        else
        {
            std::cout
                << "[PASS] Real device ID is present in audit evidence.\n";
        }

        const bool hasSerial =
            !device.getSerialNumber().empty() &&
            auditContent.find(
                device.getSerialNumber()) !=
                std::string::npos;

        if (!hasSerial)
        {
            std::cout
                << "[FAIL] Real serial number was not found in audit evidence.\n";

            passed = false;
        }
        else
        {
            std::cout
                << "[PASS] Real serial number is present in audit evidence.\n";
        }

        const bool hasActor =
            !actorId.empty() &&
            auditContent.find(actorId) !=
                std::string::npos;

        if (!hasActor)
        {
            std::cout
                << "[WARN] Windows actor ID was not found in audit evidence.\n"
                << "       Verify the generated JSONL manually.\n";
        }
        else
        {
            std::cout
                << "[PASS] Actual Windows actor ID is present.\n";
        }

        const bool hasOperation =
            !pipelineResult.sanitization.operationId.empty() &&
            auditContent.find(
                pipelineResult.sanitization.operationId) !=
                std::string::npos;

        if (!hasOperation)
        {
            std::cout
                << "[FAIL] Real operation ID was not found in audit evidence.\n";

            passed = false;
        }
        else
        {
            std::cout
                << "[PASS] Real operation ID is present in audit evidence.\n";
        }
    }

    return passed;
}

bool validateCertificateEvidence(
    const SanitizationPipelineResult& pipelineResult,
    const StorageDevice& device)
{
    separator();

    std::cout
        << "CERTIFICATE EVIDENCE VALIDATION\n"
        << "------------------------------------------------------------\n";

    bool passed = true;

    if (!pipelineResult.certificateGenerated)
    {
        std::cout
            << "[FAIL] Pipeline did not generate a certificate.\n";

        return false;
    }

    std::cout
        << "[PASS] Certificate generated by pipeline.\n";

    if (!pipelineResult.certificatePersisted)
    {
        std::cout
            << "[FAIL] Pipeline did not persist the certificate.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate persisted by pipeline.\n";
    }

    if (pipelineResult.certificatePath.empty())
    {
        std::cout
            << "[FAIL] Certificate path is empty.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "Certificate path:\n"
            << pipelineResult.certificatePath
            << '\n';

        if (!fileExistsAndNonEmpty(
                pipelineResult.certificatePath))
        {
            std::cout
                << "[FAIL] Certificate file does not exist or is empty.\n";

            passed = false;
        }
        else
        {
            std::cout
                << "[PASS] Certificate file exists and contains data.\n";
        }
    }

    const SanitizationCertificate& certificate =
        pipelineResult.certificate;

    if (certificate.certificateId.empty())
    {
        std::cout
            << "[FAIL] Certificate ID is empty.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate ID generated.\n";
    }

    if (certificate.operationId !=
        pipelineResult.sanitization.operationId)
    {
        std::cout
            << "[FAIL] Certificate operation ID does not match real operation.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate operation ID matches.\n";
    }

    if (certificate.deviceId !=
        device.getDeviceId())
    {
        std::cout
            << "[FAIL] Certificate device ID does not match real target.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate device ID matches real target.\n";
    }

    if (certificate.serialNumber !=
        device.getSerialNumber())
    {
        std::cout
            << "[FAIL] Certificate serial number does not match real target.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate serial number matches.\n";
    }

    if (certificate.capacityBytes !=
        device.getCapacityBytes())
    {
        std::cout
            << "[FAIL] Certificate capacity does not match real target.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate capacity matches.\n";
    }

    if (certificate.method !=
        pipelineResult.sanitization.method)
    {
        std::cout
            << "[FAIL] Certificate method does not match real sanitization result.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate method matches.\n";
    }

    if (!certificate.verificationPerformed)
    {
        std::cout
            << "[FAIL] Certificate reports verification was not performed.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Verification evidence is recorded.\n";
    }

    if (certificate.verificationStatus !=
        VerificationStatus::PASSED)
    {
        std::cout
            << "[FAIL] Certificate verification status is not PASSED.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate verification status is PASSED.\n";
    }

    if (!certificate.verificationPassed)
    {
        std::cout
            << "[FAIL] Certificate verificationPassed is false.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate verificationPassed is TRUE.\n";
    }

    if (certificate.certificateHash.empty())
    {
        std::cout
            << "[FAIL] Certificate SHA-256 hash is empty.\n";

        passed = false;
    }
    else if (certificate.certificateHash.size() != 64)
    {
        std::cout
            << "[FAIL] Certificate SHA-256 hash length is "
            << certificate.certificateHash.size()
            << " instead of 64.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate SHA-256 hash exists.\n";
    }

    if (!certificate.isValid())
    {
        std::cout
            << "[FAIL] Certificate isValid() returned FALSE.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Certificate isValid() returned TRUE.\n";
    }

    return passed;
}

bool runRealSystemDiskRejectionTest(
    const StorageDevice& systemDisk,
    const std::string& actorId)
{
    separator();

    std::cout
        << "CASE 1 - REAL SYSTEM DISK SAFETY REJECTION TEST\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "\nThis case uses the ACTUAL detected Windows system disk.\n"
        << "No sanitization should be executed.\n";

    printDevice(systemDisk, 1);

    const auto beforeCapacity =
        systemDisk.getCapacityBytes();

    SanitizationPipeline pipeline;

    const SanitizationPipelineResult result =
        pipeline.execute(
            systemDisk,
            {},
            actorId);

    std::cout
        << "\nPipeline status           : "
        << statusToString(result.sanitization.status)
        << '\n'
        << "Pipeline message          : "
        << result.pipelineMessage
        << '\n'
        << "Sanitization error        : "
        << result.sanitization.errorMessage
        << '\n'
        << "Bytes processed           : "
        << result.sanitization.bytesProcessed
        << '\n';

    bool passed = true;

    if (result.sanitization.status ==
        SanitizationStatus::COMPLETED)
    {
        std::cout
            << "[FAIL] System disk was reported as sanitized.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] System disk sanitization was not completed.\n";
    }

    if (result.sanitization.bytesProcessed != 0)
    {
        std::cout
            << "[FAIL] Bytes were processed on the system disk.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] No device bytes were processed.\n";
    }

    if (systemDisk.getCapacityBytes() !=
        beforeCapacity)
    {
        std::cout
            << "[FAIL] Device metadata unexpectedly changed.\n";

        passed = false;
    }

    if (!result.auditTrailPersisted)
    {
        std::cout
            << "[FAIL] Safety rejection did not persist the audit trail.\n";

        passed = false;
    }
    else
    {
        std::cout
            << "[PASS] Safety rejection was audited.\n";
    }

    return passed;
}

int runRealPipelineTest(
    const StorageDevice& selectedDevice,
    const std::string& actorId)
{
    separator();

    std::cout
        << "CASE 2 - REAL SACRIFICIAL DEVICE PIPELINE TEST\n"
        << "------------------------------------------------------------\n";

    printDevice(selectedDevice, 1);

    if (selectedDevice.isSystemDisk())
    {
        std::cout
            << "\n[FAIL] Selected device is the Windows system disk.\n"
            << "Refusing to continue.\n";

        return 10;
    }

    if (selectedDevice.getDeviceId().empty())
    {
        std::cout
            << "\n[FAIL] Device ID is empty.\n";

        return 10;
    }

    if (selectedDevice.getSerialNumber().empty())
    {
        std::cout
            << "\n[FAIL] Serial number is empty.\n";

        return 10;
    }

    if (selectedDevice.getCapacityBytes() == 0)
    {
        std::cout
            << "\n[FAIL] Device capacity is zero.\n";

        return 10;
    }

    SanitizationPipeline pipeline;

    /*
     * requestId is deliberately empty here.
     *
     * This standalone executable is not connected to the Web backend,
     * so there is no real backend sanitization request to pass.
     *
     * We do NOT manufacture a fake MongoDB request ID.
     *
     * actorId is the actual Windows account running this test.
     */

    const SanitizationPipelineResult result =
        pipeline.execute(
            selectedDevice,
            {},
            actorId);

    separator();

    std::cout
        << "REAL PIPELINE RESULT\n"
        << "------------------------------------------------------------\n"
        << "Operation ID             : "
        << result.sanitization.operationId
        << '\n'
        << "Device ID                : "
        << result.sanitization.deviceId
        << '\n'
        << "Serial                   : "
        << result.sanitization.serialNumber
        << '\n'
        << "Method                   : "
        << methodToString(
               result.sanitization.method)
        << '\n'
        << "Status                   : "
        << statusToString(
               result.sanitization.status)
        << '\n'
        << "Verification             : "
        << verificationToString(
               result.sanitization.verificationStatus)
        << '\n'
        << "Verification performed   : "
        << (result.sanitization.verificationPerformed
                ? "YES"
                : "NO")
        << '\n'
        << "Verification samples     : "
        << result.sanitization.verificationSamples
        << '\n'
        << "Bytes processed          : "
        << result.sanitization.bytesProcessed
        << '\n'
        << "Bytes verified           : "
        << result.sanitization.bytesVerified
        << '\n'
        << "Duration                 : "
        << result.sanitization.operationDurationMs
        << " ms\n"
        << "Message                  : "
        << result.sanitization.message
        << '\n'
        << "Error                    : "
        << result.sanitization.errorMessage
        << '\n'
        << "Audit persisted          : "
        << (result.auditTrailPersisted
                ? "YES"
                : "NO")
        << '\n'
        << "Certificate generated    : "
        << (result.certificateGenerated
                ? "YES"
                : "NO")
        << '\n'
        << "Certificate persisted    : "
        << (result.certificatePersisted
                ? "YES"
                : "NO")
        << '\n';

    if (result.sanitization.method !=
        SanitizationMethod::HostOverwrite)
    {
        std::cout
            << "\n[STOP]\n"
            << "The real device was not assigned Host Overwrite by\n"
            << "SanitizationEngine.\n"
            << "This executable is specifically validating the\n"
            << "Host Overwrite vertical pipeline.\n"
            << "No second sanitization method will be forced.\n";

        return 3;
    }

    if (!result.sanitization.isSuccess())
    {
        std::cout
            << "\n[FAIL] Real sanitization + verification did not pass.\n";

        return 4;
    }

    if (!result.auditTrailPersisted)
    {
        std::cout
            << "\n[FAIL] Audit trail was not fully persisted.\n";

        return 5;
    }

    if (!result.certificateGenerated)
    {
        std::cout
            << "\n[FAIL] Pipeline did not generate certificate.\n";

        return 6;
    }

    if (!result.certificatePersisted)
    {
        std::cout
            << "\n[FAIL] Pipeline did not persist certificate.\n";

        return 7;
    }

    const bool auditPassed =
        validateAuditEvidence(
            result,
            selectedDevice,
            actorId);

    if (!auditPassed)
    {
        std::cout
            << "\n[FAIL] Audit evidence validation failed.\n";

        return 8;
    }

    const bool certificatePassed =
        validateCertificateEvidence(
            result,
            selectedDevice);

    if (!certificatePassed)
    {
        std::cout
            << "\n[FAIL] Certificate evidence validation failed.\n";

        return 9;
    }

    separator();

    std::cout
        << "REAL SANITIZATION PIPELINE TEST PASSED\n"
        << "------------------------------------------------------------\n"
        << "Physical device sanitization : PASS\n"
        << "Host Overwrite               : PASS\n"
        << "Post-write verification      : PASS\n"
        << "Audit persistence            : PASS\n"
        << "Certificate generation       : PASS\n"
        << "Certificate persistence      : PASS\n"
        << "Certificate validation       : PASS\n";

    if (!result.sanitization.operationId.empty())
    {
        std::cout
            << "\nOperation ID:\n"
            << result.sanitization.operationId
            << '\n';
    }

    if (!result.certificate.certificateId.empty())
    {
        std::cout
            << "\nCertificate ID:\n"
            << result.certificate.certificateId
            << '\n';
    }

    if (!result.certificate.certificateHash.empty())
    {
        std::cout
            << "\nCertificate SHA-256:\n"
            << result.certificate.certificateHash
            << '\n';
    }

    std::cout
        << "\nAudit log:\n"
        << result.auditLogPath
        << '\n'
        << "\nCertificate file:\n"
        << result.certificatePath
        << '\n';

    return 0;
}
}

int main()
{
    separator();

    std::cout
        << "       SECUREWIPE SANITIZATION PIPELINE AUDIT TEST\n"
        << "       REAL WINDOWS DEVICES - NO FAKE DEVICE DATA\n";

    separator();

    std::cout
        << "\nIMPORTANT\n"
        << "------------------------------------------------------------\n"
        << "CASE 1 is non-destructive and verifies system-disk rejection.\n"
        << "CASE 2 is DESTRUCTIVE and permanently destroys the selected\n"
        << "physical device data.\n"
        << "\nUse ONLY a disposable/sacrificial physical device for CASE 2.\n"
        << "Never use your Windows system disk.\n";

    const std::string actorId =
        getWindowsUserName();

    std::cout
        << "\nActual Windows actor:\n"
        << actorId
        << '\n';

    // ---------------------------------------------------------
    // REAL DEVICE DISCOVERY
    // ---------------------------------------------------------

    separator();

    std::cout
        << "REAL DEVICE DISCOVERY\n"
        << "------------------------------------------------------------\n";

    WindowsStorageDiscovery discovery;

    std::vector<StorageDevice> devices;

    try
    {
        devices = discovery.discover();
    }
    catch (const std::exception& exception)
    {
        std::cout
            << "\n[FAIL] Device discovery threw an exception:\n"
            << exception.what()
            << '\n';

        return 1;
    }
    catch (...)
    {
        std::cout
            << "\n[FAIL] Device discovery threw an unknown exception.\n";

        return 1;
    }

    if (devices.empty())
    {
        std::cout
            << "\n[FAIL] No real physical storage devices were detected.\n";

        return 1;
    }

    std::cout
        << "\nDetected "
        << devices.size()
        << " physical storage device(s).\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
        printDevice(devices[i], i + 1);

    // ---------------------------------------------------------
    // LOCATE REAL SYSTEM DISK
    // ---------------------------------------------------------

    const auto systemDiskIt =
        std::find_if(
            devices.begin(),
            devices.end(),
            [](const StorageDevice& device)
            {
                return device.isSystemDisk();
            });

    if (systemDiskIt == devices.end())
    {
        std::cout
            << "\n[WARN] No device was identified as the Windows system disk.\n"
            << "CASE 1 cannot be safely executed.\n";

        return 2;
    }

    const StorageDevice systemDisk =
        *systemDiskIt;

    // ---------------------------------------------------------
    // CASE 1
    // ---------------------------------------------------------

    const bool safetyCasePassed =
        runRealSystemDiskRejectionTest(
            systemDisk,
            actorId);

    if (!safetyCasePassed)
    {
        separator();

        std::cout
            << "CASE 1 FAILED\n";

        return 3;
    }

    separator();

    std::cout
        << "CASE 1 PASSED\n"
        << "Real system-disk protection is working.\n";

    // ---------------------------------------------------------
    // CASE 2 TARGET SELECTION
    // ---------------------------------------------------------

    separator();

    std::cout
        << "CASE 2 - SACRIFICIAL DEVICE SELECTION\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "\nSelect the REAL SACRIFICIAL device to wipe.\n"
        << "Do not select the Windows system disk.\n"
        << "\nEnter device number: ";

    std::string input;
    std::getline(std::cin, input);

    std::size_t selectedIndex = 0;

    if (!parseIndex(
            input,
            devices.size(),
            selectedIndex))
    {
        std::cout
            << "\n[FAIL] Invalid device selection.\n";

        return 4;
    }

    StorageDevice selectedDevice =
        devices[selectedIndex];

    printDevice(
        selectedDevice,
        selectedIndex + 1);

    // ---------------------------------------------------------
    // HARD SAFETY GUARDS
    // ---------------------------------------------------------

    if (selectedDevice.isSystemDisk())
    {
        separator();

        std::cout
            << "CRITICAL SAFETY BLOCK\n"
            << "------------------------------------------------------------\n"
            << "The selected device is the Windows system disk.\n"
            << "The destructive pipeline will NOT run.\n";

        return 5;
    }

    if (selectedDevice.getDeviceId().empty())
    {
        std::cout
            << "\n[FAIL] Real device ID is empty.\n";

        return 5;
    }

    if (selectedDevice.getSerialNumber().empty())
    {
        std::cout
            << "\n[FAIL] Real serial number is empty.\n";

        return 5;
    }

    if (selectedDevice.getCapacityBytes() == 0)
    {
        std::cout
            << "\n[FAIL] Real device capacity is zero.\n";

        return 5;
    }

    // ---------------------------------------------------------
    // EXPLICIT SERIAL CONFIRMATION
    // ---------------------------------------------------------

    separator();

    std::cout
        << "DESTRUCTIVE CONFIRMATION\n"
        << "------------------------------------------------------------\n"
        << "\nTarget device:\n"
        << selectedDevice.getDeviceId()
        << "\n\nModel:\n"
        << selectedDevice.getModel()
        << "\n\nSerial:\n"
        << selectedDevice.getSerialNumber()
        << "\n\nCapacity:\n"
        << selectedDevice.getCapacityBytes()
        << " bytes\n\n"
        << "This operation will permanently overwrite the physical device.\n"
        << "Type the EXACT serial number to continue:\n";

    std::string serialConfirmation;
    std::getline(
        std::cin,
        serialConfirmation);

    if (trim(serialConfirmation) !=
        selectedDevice.getSerialNumber())
    {
        std::cout
            << "\n[STOP] Serial-number confirmation failed.\n"
            << "No sanitization was started.\n";

        return 6;
    }

    std::cout
        << "\nType START HOST OVERWRITE to begin:\n";

    std::string destructiveConfirmation;
    std::getline(
        std::cin,
        destructiveConfirmation);

    if (toUpper(
            trim(destructiveConfirmation)) !=
        "START HOST OVERWRITE")
    {
        std::cout
            << "\n[STOP] Final destructive confirmation failed.\n"
            << "No sanitization was started.\n";

        return 7;
    }

    // ---------------------------------------------------------
    // CASE 2 REAL PIPELINE
    // ---------------------------------------------------------

    return runRealPipelineTest(
        selectedDevice,
        actorId);
}