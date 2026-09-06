#include "SanitizationEngine.h"


#include <iostream>
#include <string>

namespace
{
int passed = 0;
int failed = 0;

void check(
    const std::string &testName,
    SanitizationMethod expected,
    SanitizationMethod actual)
{
    const bool success = expected == actual;

    std::cout << "\n[" << (success ? "PASS" : "FAIL") << "] "
              << testName << '\n';

    std::cout << "    Expected : "
              << static_cast<int>(expected) << '\n';

    std::cout << "    Actual   : "
              << static_cast<int>(actual) << '\n';

    if (success)
        ++passed;
    else
        ++failed;
}
}

int main()
{
    std::cout << "\n============================================================\n";
    std::cout << "        SECUREWIPE SANITIZATION METHOD TEST\n";
    std::cout << "============================================================\n";

    SanitizationEngine engine;

    // --------------------------------------------------------
    // TEST 1: NVMe + Native Sanitize
    // --------------------------------------------------------

    {
        StorageDevice device(
            R"(\\.\PhysicalDrive1)",
            "Test NVMe",
            "NVME-TEST",
            100ULL * 1024 * 1024,
            "NVMe",
            false,
            false,
            false);

        SanitizationCapability capability;
        capability.nvmeIdentifyAvailable = true;
        capability.nativeSanitizeSupported =
            NativeSanitizeSupport::SUPPORTED;
        capability.nvmeCryptoEraseSupported = true;

        check(
            "NVMe with native sanitize capability",
            SanitizationMethod::NvmeSanitize,
            engine.selectMethod(device, capability));
    }

    // --------------------------------------------------------
    // TEST 2: SATA + ATA Sanitize
    // --------------------------------------------------------

    {
        StorageDevice device(
            R"(\\.\PhysicalDrive2)",
            "Test SATA",
            "SATA-TEST",
            100ULL * 1024 * 1024,
            "SATA",
            false,
            false,
            true);

        SanitizationCapability capability;
        capability.ataIdentifyAvailable = true;
        capability.atasanitizeSupported = true;
        capability.atacryptoScrambleSupported = true;

        check(
            "SATA with ATA SANITIZE capability",
            SanitizationMethod::AtaSanitize,
            engine.selectMethod(device, capability));
    }

    // --------------------------------------------------------
    // TEST 3: SATA + No ATA Sanitize
    // --------------------------------------------------------

    {
        StorageDevice device(
            R"(\\.\PhysicalDrive3)",
            "Test SATA HDD",
            "SATA-HDD-TEST",
            100ULL * 1024 * 1024,
            "SATA",
            false,
            false,
            true);

        SanitizationCapability capability;
        capability.ataIdentifyAvailable = true;
        capability.atasanitizeSupported = false;

        check(
            "SATA without ATA SANITIZE capability",
            SanitizationMethod::HostOverwrite,
            engine.selectMethod(device, capability));
    }

    // --------------------------------------------------------
    // TEST 4: USB + SCSI Path
    // --------------------------------------------------------

    {
        StorageDevice device(
            R"(\\.\PhysicalDrive4)",
            "Test USB",
            "USB-TEST",
            100ULL * 1024 * 1024,
            "USB",
            false,
            true,
            false);

        SanitizationCapability capability;
        capability.isUsbDevice = true;
        capability.scsiPathAvailable = true;

        check(
            "USB with available SCSI path",
            SanitizationMethod::HostOverwrite,
            engine.selectMethod(device, capability));
    }

    // --------------------------------------------------------
    // TEST 5: Unsupported Device
    // --------------------------------------------------------

    {
        StorageDevice device(
            R"(\\.\PhysicalDrive5)",
            "Unknown Device",
            "UNKNOWN-TEST",
            100ULL * 1024 * 1024,
            "Unknown",
            false,
            false,
            false);

        SanitizationCapability capability;

        check(
            "Unsupported device/interface",
            SanitizationMethod::Unsupported,
            engine.selectMethod(device, capability));
    }

    // --------------------------------------------------------
    // SUMMARY
    // --------------------------------------------------------

    std::cout << "\n============================================================\n";
    std::cout << "                    TEST SUMMARY\n";
    std::cout << "============================================================\n";

    std::cout << "Total Tests : " << (passed + failed) << '\n';
    std::cout << "Passed      : " << passed << '\n';
    std::cout << "Failed      : " << failed << '\n';

    std::cout << "============================================================\n";

    return failed == 0 ? 0 : 1;
}