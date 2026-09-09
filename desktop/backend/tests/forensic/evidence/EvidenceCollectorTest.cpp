#include "EvidenceCollector.h"
#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{

void separator()
{
    std::cout << "\n============================================================\n";
}

bool parseIndex(const std::string &input, std::size_t count, std::size_t &index)
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

std::string readLine()
{
    std::string value;
    std::getline(std::cin, value);
    return value;
}

void printDevice(const StorageDevice &device, std::size_t number)
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

void printArtifact(const EvidenceItem &item, std::size_t number)
{
    std::cout
        << "\n------------------------------------------------------------\n"
        << "Artifact #" << number << '\n'
        << "------------------------------------------------------------\n"
        << "Artifact ID       : " << item.artifactId << '\n'
        << "Source            : " << item.source << '\n'
        << "Offset            : " << item.offset << '\n'
        << "Size              : " << item.size << " bytes\n"
        << "File name         : " << item.fileName << '\n'
        << "File type         : " << item.fileType << '\n'
        << "Recovered path    : " << item.recoveredPath << '\n'
        << "Recovered         : " << (item.recovered ? "YES" : "NO") << '\n'
        << "Validated         : " << (item.validated ? "YES" : "NO") << '\n'
        << "SHA-256           : " << item.sha256 << '\n'
        << "Confidence score  : " << item.confidenceScore << '\n'
        << "Confidence        : " << item.getConfidenceString() << '\n';

    std::cout << "Confidence reasons:\n";
    for (const auto &reason : item.confidenceReasons)
        std::cout << "  - " << reason << '\n';
}

bool isDefinitelyUnsafeForThisTest(const StorageDevice &device)
{
    if (device.isSystemDisk())
        return true;

    const std::string &deviceId = device.getDeviceId();
    return deviceId.empty();
}

} // namespace

int main()
{
    separator();
    std::cout << "SECUREWIPE FORENSIC REAL PHYSICAL-DEVICE TEST\n";
    std::cout << "Discovery -> Device Selection -> Read-Only Raw Scan -> JPEG Carving -> Validation -> SHA-256 -> Confidence\n";
    separator();

    std::cout
        << "\nThis test uses the REAL physical storage devices detected by SecureWipe.\n"
        << "It does NOT require you to provide a JPEG file.\n"
        << "The selected device is opened through the existing EvidenceCollector\n"
        << "in READ-ONLY mode and scanned chunk by chunk.\n\n"
        << "IMPORTANT:\n"
        << "- NEVER select your Windows system disk.\n"
        << "- Use only a disposable/test storage device.\n"
        << "- A complete scan of a large disk can take a long time.\n";

    // =========================================================
    // 1. REAL DEVICE DISCOVERY
    // =========================================================
    separator();
    std::cout << "STEP 1 - DISCOVER CONNECTED STORAGE DEVICES\n";
    std::cout << "------------------------------------------------------------\n";

    WindowsStorageDiscovery discovery;
    const std::vector<StorageDevice> devices = discovery.discover();

    if (devices.empty())
    {
        std::cout << "[FAIL] No physical storage devices were discovered.\n";
        return 1;
    }

    std::cout << "Discovered " << devices.size() << " physical device(s).\n";

    for (std::size_t i = 0; i < devices.size(); ++i)
        printDevice(devices[i], i + 1);

    // =========================================================
    // 2. DEVICE SELECTION
    // =========================================================
    separator();
    std::cout << "STEP 2 - SELECT THE REAL TEST DEVICE\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Enter device number: ";

    std::size_t selectedIndex = 0;
    if (!parseIndex(readLine(), devices.size(), selectedIndex))
    {
        std::cout << "[FAIL] Invalid device selection.\n";
        return 1;
    }

    const StorageDevice &selectedDevice = devices[selectedIndex];

    separator();
    std::cout << "SELECTED DEVICE\n";
    std::cout << "------------------------------------------------------------\n";
    printDevice(selectedDevice, selectedIndex + 1);

    if (selectedDevice.isSystemDisk())
    {
        std::cout << "\n[BLOCKED] This device is marked as the Windows system disk.\n";
        std::cout << "No forensic scan was started.\n";
        return 2;
    }

    if (selectedDevice.getDeviceId().empty())
    {
        std::cout << "\n[FAIL] Selected device has no physical device path.\n";
        return 1;
    }

    // =========================================================
    // 3. EXPLICIT READ-ONLY CONFIRMATION
    // =========================================================
    separator();
    std::cout << "STEP 3 - READ-ONLY ACQUISITION CONFIRMATION\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout
        << "Selected physical source:\n"
        << "  " << selectedDevice.getDeviceId() << "\n\n"
        << "The EvidenceCollector will open this source for READ ONLY access.\n"
        << "No sanitization, overwrite, delete, or write operation is performed\n"
        << "by this test.\n\n"
        << "Type exactly: START FORENSIC READ\n"
        << "Confirmation: ";

    if (readLine() != "START FORENSIC READ")
    {
        std::cout << "\n[ABORTED] Read-only acquisition was not authorized.\n";
        return 3;
    }

    // =========================================================
    // 4. REAL FORENSIC COLLECTION
    // =========================================================
    separator();
    std::cout << "STEP 4 - REAL FORENSIC ACQUISITION\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Source: " << selectedDevice.getDeviceId() << '\n';
    std::cout << "Starting EvidenceCollector...\n\n";

    EvidenceCollector collector;

    const EvidenceCollectionResult result =
        collector.collectWithSummary(selectedDevice.getDeviceId());

    const EvidenceCollectionSummary &summary =
        result.summary;

    // =========================================================
    // 5. COLLECTION SUMMARY
    // =========================================================
    separator();
    std::cout << "STEP 5 - ACQUISITION SUMMARY\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Source opened       : " << (summary.sourceOpened ? "PASS" : "FAIL") << '\n';
    std::cout << "Scan completed      : " << (summary.completed ? "PASS" : "FAIL") << '\n';
    std::cout << "Total bytes         : " << summary.totalBytes << '\n';
    std::cout << "Bytes scanned       : " << summary.bytesScanned << '\n';
    std::cout << "Candidates found    : " << summary.candidatesFound << '\n';
    std::cout << "Recovered artifacts : " << summary.recoveredArtifacts << '\n';
    std::cout << "Validated artifacts : " << summary.validatedArtifacts << '\n';
    std::cout << "Rejected artifacts  : " << summary.rejectedArtifacts << '\n';
    std::cout << "High confidence     : " << summary.highConfidenceArtifacts << '\n';
    std::cout << "Recovered bytes     : " << summary.recoveredBytes << '\n';

    if (!summary.sourceOpened)
    {
        std::cout << "\n[FAIL] EvidenceCollector could not open the selected physical device.\n";
        return 4;
    }

    if (!summary.completed)
    {
        std::cout << "\n[FAIL] Physical-device acquisition did not complete.\n";
        return 5;
    }

    // =========================================================
    // 6. RECOVERED ARTIFACTS
    // =========================================================
    separator();
    std::cout << "STEP 6 - RECOVERED FORENSIC ARTIFACTS\n";
    std::cout << "------------------------------------------------------------\n";

    if (result.evidence.empty())
    {
        std::cout
            << "No validated artifacts were recovered from the selected device.\n\n"
            << "Physical read test       : PASS\n"
            << "Chunk-by-chunk scan      : PASS\n"
            << "JPEG carving candidates : " << summary.candidatesFound << '\n'
            << "Validated evidence      : 0\n\n"
            << "The scan itself completed successfully, but the current device\n"
            << "did not contain a JPEG artifact accepted by the validation pipeline.\n";

        return 0;
    }

    for (std::size_t i = 0; i < result.evidence.size(); ++i)
        printArtifact(result.evidence[i], i + 1);

    // =========================================================
    // 7. FINAL ENGINE ASSERTIONS
    // =========================================================
    separator();
    std::cout << "STEP 7 - FORENSIC ENGINE ASSERTIONS\n";
    std::cout << "------------------------------------------------------------\n";

    bool passed = true;

    for (const auto &item : result.evidence)
    {
        if (!item.recovered)
        {
            std::cout << "[FAIL] Artifact " << item.artifactId << " is not marked recovered.\n";
            passed = false;
        }
        else
        {
            std::cout << "[PASS] " << item.artifactId << " recovered.\n";
        }

        if (!item.validated)
        {
            std::cout << "[FAIL] Artifact " << item.artifactId << " failed validation.\n";
            passed = false;
        }
        else
        {
            std::cout << "[PASS] " << item.artifactId << " validated.\n";
        }

        if (item.fileType != "JPEG")
        {
            std::cout << "[FAIL] Artifact " << item.artifactId << " is not JPEG.\n";
            passed = false;
        }
        else
        {
            std::cout << "[PASS] " << item.artifactId << " identified as JPEG.\n";
        }

        if (item.sha256.empty() || item.sha256.length() != 64)
        {
            std::cout << "[FAIL] Artifact " << item.artifactId << " has invalid SHA-256.\n";
            passed = false;
        }
        else
        {
            std::cout << "[PASS] " << item.artifactId << " has SHA-256.\n";
        }

        if (item.confidence != ForensicConfidence::HIGH)
        {
            std::cout << "[FAIL] Artifact " << item.artifactId << " is not HIGH confidence.\n";
            passed = false;
        }
        else
        {
            std::cout << "[PASS] " << item.artifactId << " is HIGH confidence.\n";
        }
    }

    separator();

    if (!passed)
    {
        std::cout << "FORENSIC PHYSICAL-DEVICE TEST FAILED\n";
        return 6;
    }

    std::cout << "REAL FORENSIC PHYSICAL-DEVICE TEST PASSED\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Device Discovery      : PASS\n";
    std::cout << "Target Selection      : PASS\n";
    std::cout << "Read-Only Source Open : PASS\n";
    std::cout << "Chunk-by-Chunk Scan   : PASS\n";
    std::cout << "JPEG Recovery         : PASS\n";
    std::cout << "JPEG Validation       : PASS\n";
    std::cout << "SHA-256               : PASS\n";
    std::cout << "Confidence Scoring    : PASS\n";
    std::cout << "Validated Artifacts   : " << result.evidence.size() << '\n';

    separator();
    return 0;
}
