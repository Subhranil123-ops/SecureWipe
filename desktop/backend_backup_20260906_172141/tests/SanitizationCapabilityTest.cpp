#include "WindowsStorageDiscovery.h"
#include "StorageDevice.h"
#include "SafetyEngine.h"
#include "SafetyResult.h"
#include "SanitizationCapability.h"
#include "SanitizationEngine.h"
#include "SanitizationMethod.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <limits>

static std::string getMethodName(SanitizationMethod method)
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

static std::string getNativeSupportName(
    NativeSanitizeSupport support)
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

static std::string formatCapacity(
    std::uint64_t bytes)
{
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;
    constexpr double TB = GB * 1024.0;

    std::ostringstream output;

    output << std::fixed
           << std::setprecision(2);

    if (bytes >= static_cast<std::uint64_t>(TB))
    {
        output << static_cast<double>(bytes) / TB
               << " TB";
    }
    else if (bytes >= static_cast<std::uint64_t>(GB))
    {
        output << static_cast<double>(bytes) / GB
               << " GB";
    }
    else if (bytes >= static_cast<std::uint64_t>(MB))
    {
        output << static_cast<double>(bytes) / MB
               << " MB";
    }
    else if (bytes >= static_cast<std::uint64_t>(KB))
    {
        output << static_cast<double>(bytes) / KB
               << " KB";
    }
    else
    {
        output << bytes
               << " bytes";
    }

    return output.str();
}

static void printSeparator()
{
    std::cout
        << "============================================================\n";
}

static void printDevice(
    const StorageDevice& device,
    std::size_t index)
{
    std::cout
        << "\n[" << index << "]\n";

    std::cout
        << "Device ID       : "
        << device.getDeviceId()
        << '\n';

    std::cout
        << "Model           : "
        << device.getModel()
        << '\n';

    std::cout
        << "Serial Number   : "
        << device.getSerialNumber()
        << '\n';

    std::cout
        << "Capacity        : "
        << formatCapacity(device.getCapacityBytes())
        << " ("
        << device.getCapacityBytes()
        << " bytes)\n";

    std::cout
        << "Interface       : "
        << device.getInterfaceType()
        << '\n';

    std::cout
        << "System Disk     : "
        << (device.isSystemDisk() ? "YES" : "NO")
        << '\n';

    std::cout
        << "Removable       : "
        << (device.isRemovable() ? "YES" : "NO")
        << '\n';

    std::cout
        << "Seek Penalty    : "
        << (device.hasSeekPenalty() ? "YES" : "NO")
        << '\n';
}

static void printSafetyResult(
    const SafetyResult& result)
{
    std::cout << '\n';

    printSeparator();

    std::cout
        << "                    SAFETY ENGINE\n";

    printSeparator();

    for (const auto& check : result.checks)
    {
        std::cout
            << std::left
            << std::setw(28)
            << check.checkName
            << " : "
            << (check.passed ? "PASS" : "FAIL")
            << '\n';

        std::cout
            << "    "
            << check.message
            << '\n';
    }

    printSeparator();

    std::cout
        << "Overall Decision : "
        << result.decision
        << '\n';

    std::cout
        << "Summary          : "
        << result.summary
        << '\n';

    printSeparator();
}

static void printCapability(
    const StorageDevice& device,
    const SanitizationCapability& capability)
{
    std::cout << '\n';

    printSeparator();

    std::cout
        << "             SANITIZATION CAPABILITY\n";

    printSeparator();

    std::cout
        << "Interface detected : "
        << device.getInterfaceType()
        << '\n';

    std::cout
        << "\n--- General / USB ---\n";

    std::cout
        << "USB Device                    : "
        << (capability.isUsbDevice ? "YES" : "NO")
        << '\n';

    std::cout
        << "SCSI Path Available            : "
        << (capability.scsiPathAvailable ? "YES" : "NO")
        << '\n';

    std::cout
        << "Storage Property Query        : "
        << (capability.storagePropertyQueryAvailable
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "Native Sanitize Support       : "
        << getNativeSupportName(
               capability.nativeSanitizeSupported)
        << '\n';

    std::cout
        << "\n--- NVMe ---\n";

    std::cout
        << "NVMe Identify Available       : "
        << (capability.nvmeIdentifyAvailable
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "NVMe Block Erase              : "
        << (capability.nvmeBlockEraseSupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "NVMe Crypto Erase             : "
        << (capability.nvmeCryptoEraseSupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "NVMe Overwrite                : "
        << (capability.nvmeOverwriteSupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "\n--- ATA / SATA ---\n";

    std::cout
        << "ATA Identify Available        : "
        << (capability.ataIdentifyAvailable
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "ATA Security Supported        : "
        << (capability.ataSecuritySupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "ATA Enhanced Erase            : "
        << (capability.ataEnhancedEraseSupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "ATA Security Enabled          : "
        << (capability.ataSecurityEnabled
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "ATA Security Locked           : "
        << (capability.ataSecurityLocked
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "ATA Security Frozen           : "
        << (capability.ataSecurityFrozen
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "\n--- ATA SANITIZE ---\n";

    std::cout
        << "ATA SANITIZE Supported        : "
        << (capability.atasanitizeSupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "Crypto Scramble EXT           : "
        << (capability.atacryptoScrambleSupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "Block Erase EXT               : "
        << (capability.atablockEraseSupported
                ? "YES"
                : "NO")
        << '\n';

    std::cout
        << "Overwrite EXT                 : "
        << (capability.ataoverwriteSupported
                ? "YES"
                : "NO")
        << '\n';

    printSeparator();
}

static bool validateCapabilityConsistency(
    const StorageDevice& device,
    const SanitizationCapability& capability)
{
    bool valid = true;

    std::cout << '\n';

    printSeparator();

    std::cout
        << "             CAPABILITY CONSISTENCY\n";

    printSeparator();

    if (device.getInterfaceType() == "USB")
    {
        if (!capability.isUsbDevice)
        {
            std::cout
                << "FAIL: Interface is USB but "
                   "isUsbDevice is false.\n";

            valid = false;
        }
        else
        {
            std::cout
                << "PASS: USB interface correctly propagated.\n";
        }
    }

    if (device.getInterfaceType() == "NVMe")
    {
        if (!capability.nvmeIdentifyAvailable)
        {
            std::cout
                << "WARNING: NVMe Identify is unavailable.\n";
        }
        else
        {
            std::cout
                << "PASS: NVMe Identify path is available.\n";
        }
    }

    if (device.getInterfaceType() == "SATA")
    {
        if (!capability.ataIdentifyAvailable)
        {
            std::cout
                << "WARNING: ATA Identify is unavailable.\n";
        }
        else
        {
            std::cout
                << "PASS: ATA Identify path is available.\n";
        }

        if (capability.atasanitizeSupported)
        {
            bool algorithmAvailable =
                capability.atacryptoScrambleSupported ||
                capability.atablockEraseSupported ||
                capability.ataoverwriteSupported;

            if (!algorithmAvailable)
            {
                std::cout
                    << "FAIL: ATA SANITIZE is reported "
                       "supported, but no ATA sanitize "
                       "algorithm is available.\n";

                valid = false;
            }
            else
            {
                std::cout
                    << "PASS: ATA SANITIZE has at least "
                       "one available algorithm.\n";
            }
        }
    }

    if (device.getInterfaceType() != "USB" &&
        device.getInterfaceType() != "NVMe" &&
        device.getInterfaceType() != "SATA")
    {
        std::cout
            << "INFO: No protocol-specific consistency "
               "rules are defined for this interface.\n";
    }

    printSeparator();

    std::cout
        << "Capability Consistency : "
        << (valid ? "PASS" : "FAIL")
        << '\n';

    printSeparator();

    return valid;
}

static bool selectAtaAlgorithm(
    const SanitizationCapability& capability,
    std::string& algorithm)
{
    if (capability.atacryptoScrambleSupported)
    {
        algorithm = "Crypto Scramble EXT";
        return true;
    }

    if (capability.atablockEraseSupported)
    {
        algorithm = "Block Erase EXT";
        return true;
    }

    if (capability.ataoverwriteSupported)
    {
        algorithm = "Overwrite EXT";
        return true;
    }

    algorithm = "None";

    return false;
}

static bool selectNvmeAlgorithm(
    const SanitizationCapability& capability,
    std::string& algorithm)
{
    if (capability.nvmeCryptoEraseSupported)
    {
        algorithm = "Crypto Erase";
        return true;
    }

    if (capability.nvmeBlockEraseSupported)
    {
        algorithm = "Block Erase";
        return true;
    }

    if (capability.nvmeOverwriteSupported)
    {
        algorithm = "Overwrite";
        return true;
    }

    algorithm = "None";

    return false;
}

int main()
{
    std::cout << '\n';

    printSeparator();

    std::cout
        << "       SECUREWIPE REAL DEVICE FUNCTIONAL TEST\n";

    printSeparator();

    std::cout
        << "\nIMPORTANT:\n"
        << "This test performs REAL device discovery and\n"
        << "REAL sanitization capability detection.\n"
        << "No destructive sanitization will be executed.\n";

    printSeparator();

    // ============================================================
    // STEP 1: DEVICE DISCOVERY
    // ============================================================

    std::cout
        << "\n[1] DEVICE DISCOVERY\n";

    printSeparator();

    WindowsStorageDiscovery discovery;

    std::vector<StorageDevice> devices =
        discovery.discover();

    if (devices.empty())
    {
        std::cerr
            << "\nERROR: No physical storage devices "
               "were discovered.\n";

        return 1;
    }

    std::cout
        << "\nDiscovered "
        << devices.size()
        << " physical storage device(s).\n";

    // ============================================================
    // STEP 2: DISPLAY DEVICES
    // ============================================================

    std::cout
        << "\n[2] AVAILABLE PHYSICAL DEVICES\n";

    printSeparator();

    for (std::size_t i = 0;
         i < devices.size();
         ++i)
    {
        printDevice(devices[i], i);
    }

    printSeparator();

    // ============================================================
    // STEP 3: SELECT TARGET
    // ============================================================

    std::cout
        << "\n[3] TARGET DEVICE SELECTION\n";

    printSeparator();

    std::cout
        << "Enter the device number to test: ";

    std::size_t selectedIndex;

    if (!(std::cin >> selectedIndex))
    {
        std::cerr
            << "ERROR: Invalid input.\n";

        return 1;
    }

    if (selectedIndex >= devices.size())
    {
        std::cerr
            << "ERROR: Invalid device number.\n";

        return 1;
    }

    StorageDevice target =
        devices[selectedIndex];

    std::cout
        << "\nSelected Target:\n";

    printDevice(
        target,
        selectedIndex);

    // ============================================================
    // SYSTEM DISK PROTECTION
    // ============================================================

    if (target.isSystemDisk())
    {
        std::cout << '\n';

        printSeparator();

        std::cout
            << "WARNING: SELECTED DEVICE IS THE "
               "WINDOWS SYSTEM DISK.\n";

        std::cout
            << "This test will NOT continue.\n";

        printSeparator();

        return 2;
    }

    // ============================================================
    // STEP 4: TARGET IDENTITY LOCK
    // ============================================================

    std::cout
        << "\n[4] TARGET IDENTITY LOCK\n";

    printSeparator();

    SafetyEngine safetyEngine;

    safetyEngine.setExpectedTarget(
        target);

    std::cout
        << "Target identity captured successfully.\n";

    std::cout
        << "Device ID : "
        << target.getDeviceId()
        << '\n';

    std::cout
        << "Model     : "
        << target.getModel()
        << '\n';

    std::cout
        << "Serial    : "
        << target.getSerialNumber()
        << '\n';

    std::cout
        << "Capacity  : "
        << target.getCapacityBytes()
        << " bytes\n";

    printSeparator();

    // ============================================================
    // STEP 5: SAFETY ENGINE
    // ============================================================

    std::cout
        << "\n[5] SAFETY VALIDATION\n";

    SafetyResult safetyResult =
        safetyEngine.evaluateWithResult(
            target);

    printSafetyResult(
        safetyResult);

    if (!safetyResult.isOverallSafe)
    {
        std::cout
            << "\nFINAL RESULT: BLOCKED BY "
               "SAFETY ENGINE.\n";

        std::cout
            << "No sanitization will be attempted.\n";

        return 3;
    }

    // ============================================================
    // STEP 6: CAPABILITY DETECTION
    // ============================================================

    std::cout
        << "\n[6] SANITIZATION CAPABILITY DETECTION\n";

    SanitizationCapability capability =
        detectSanitizationCapability(
            target);

    printCapability(
        target,
        capability);

    // ============================================================
    // STEP 7: CAPABILITY VALIDATION
    // ============================================================

    std::cout
        << "\n[7] CAPABILITY VALIDATION\n";

    bool capabilityValid =
        validateCapabilityConsistency(
            target,
            capability);

    if (!capabilityValid)
    {
        std::cout
            << "\nFINAL RESULT: CAPABILITY VALIDATION FAILED.\n";

        return 4;
    }

    // ============================================================
    // STEP 8: METHOD SELECTION
    // ============================================================

    std::cout
        << "\n[8] SANITIZATION METHOD SELECTION\n";

    printSeparator();

    SanitizationEngine sanitizationEngine;

    SanitizationMethod method =
        sanitizationEngine.selectMethod(
            target,
            capability);

    std::cout
        << "Selected Method : "
        << getMethodName(method)
        << '\n';

    std::string algorithm =
        "Not Applicable";

    if (method ==
        SanitizationMethod::NvmeSanitize)
    {
        if (!selectNvmeAlgorithm(
                capability,
                algorithm))
        {
            std::cout
                << "ERROR: NVMe sanitize method "
                   "selected but no algorithm "
                   "is available.\n";

            return 5;
        }

        std::cout
            << "NVMe Algorithm  : "
            << algorithm
            << '\n';
    }
    else if (method ==
             SanitizationMethod::AtaSanitize)
    {
        if (!selectAtaAlgorithm(
                capability,
                algorithm))
        {
            std::cout
                << "ERROR: ATA sanitize method "
                   "selected but no algorithm "
                   "is available.\n";

            return 5;
        }

        std::cout
            << "ATA Algorithm   : "
            << algorithm
            << '\n';
    }
    else if (method ==
             SanitizationMethod::HostOverwrite)
    {
        algorithm =
            "Sequential Host Overwrite";

        std::cout
            << "Algorithm       : "
            << algorithm
            << '\n';
    }

    printSeparator();

    // ============================================================
    // STEP 9: FINAL READINESS CHECK
    // ============================================================

    std::cout
        << "\n[9] FINAL READINESS CHECK\n";

    printSeparator();

    bool identityValid =
        !target.getDeviceId().empty() &&
        !target.getModel().empty() &&
        !target.getSerialNumber().empty();

    bool capacityValid =
        target.getCapacityBytes() > 0;

    bool methodSupported =
        method != SanitizationMethod::Unsupported;

    bool readyForSanitization =
        safetyResult.isOverallSafe &&
        capabilityValid &&
        methodSupported &&
        !target.isSystemDisk() &&
        identityValid &&
        capacityValid;

    std::cout
        << "Safety Status       : "
        << (safetyResult.isOverallSafe
                ? "PASS"
                : "FAIL")
        << '\n';

    std::cout
        << "Capability Status   : "
        << (capabilityValid
                ? "PASS"
                : "FAIL")
        << '\n';

    std::cout
        << "Method Status       : "
        << (methodSupported
                ? "SUPPORTED"
                : "UNSUPPORTED")
        << '\n';

    std::cout
        << "System Disk         : "
        << (target.isSystemDisk()
                ? "YES - BLOCKED"
                : "NO")
        << '\n';

    std::cout
        << "Device Identity     : "
        << (identityValid
                ? "VALID"
                : "INVALID")
        << '\n';

    std::cout
        << "Capacity            : "
        << (capacityValid
                ? "VALID"
                : "INVALID")
        << '\n';

    printSeparator();

    // ============================================================
    // FINAL RESULT
    // ============================================================

    if (!readyForSanitization)
    {
        std::cout
            << "\nFINAL RESULT:\n"
            << "NOT READY FOR SANITIZATION.\n";

        std::cout
            << "The target disk has NOT been modified.\n";

        printSeparator();

        return 6;
    }

    std::cout
        << "\nFINAL RESULT:\n"
        << "DEVICE IS READY FOR SANITIZATION.\n";

    std::cout
        << "\nTarget Device      : "
        << target.getDeviceId()
        << '\n';

    std::cout
        << "Model              : "
        << target.getModel()
        << '\n';

    std::cout
        << "Serial             : "
        << target.getSerialNumber()
        << '\n';

    std::cout
        << "Capacity           : "
        << formatCapacity(
               target.getCapacityBytes())
        << '\n';

    std::cout
        << "Interface          : "
        << target.getInterfaceType()
        << '\n';

    std::cout
        << "Sanitization Method: "
        << getMethodName(method)
        << '\n';

    std::cout
        << "Algorithm          : "
        << algorithm
        << '\n';

    printSeparator();

    // ============================================================
    // SAFE TEST END
    // ============================================================

    std::cout
        << "\nDESTRUCTIVE OPERATION: NOT EXECUTED.\n";

    std::cout
        << "Functional capability test completed successfully.\n";

    std::cout
        << "No erase command was sent.\n";

    std::cout
        << "No overwrite was performed.\n";

    std::cout
        << "Target device remains unchanged.\n";

    printSeparator();

    return 0;
}