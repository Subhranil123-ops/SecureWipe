#include "AtaCapability.h"
#include "StorageDevice.h"

#include <iostream>

int main()
{
    std::cout << "\n============================================================\n";
    std::cout << "              ATA CAPABILITY TEST\n";
    std::cout << "============================================================\n";

    // Replace these values only if your test device differs.
    StorageDevice device(
        R"(\\.\PhysicalDriveX)",
        "TEST_DEVICE",
        "TEST_SERIAL",
        0,
        "SATA",
        false,
        false,
        true);

    std::cout << "\n[TEST] ATA IDENTIFY / Capability Detection\n";
    std::cout << "------------------------------------------------------------\n";

    AtaCapability capability = detectAtaCapability(device);

    std::cout << "IDENTIFY Available       : "
              << (capability.identifyAvailable ? "YES" : "NO") << '\n';

    std::cout << "Security Supported       : "
              << (capability.securitySupported ? "YES" : "NO") << '\n';

    std::cout << "Enhanced Erase Supported : "
              << (capability.enhancedEraseSupported ? "YES" : "NO") << '\n';

    std::cout << "Security Enabled         : "
              << (capability.securityEnabled ? "YES" : "NO") << '\n';

    std::cout << "Security Locked          : "
              << (capability.securityLocked ? "YES" : "NO") << '\n';

    std::cout << "Security Frozen          : "
              << (capability.securityFrozen ? "YES" : "NO") << '\n';

    std::cout << "SANITIZE Supported       : "
              << (capability.sanitizeSupported ? "YES" : "NO") << '\n';

    std::cout << "Crypto Scramble EXT      : "
              << (capability.cryptoScrambleSupported ? "YES" : "NO") << '\n';

    std::cout << "Block Erase EXT          : "
              << (capability.blockEraseSupported ? "YES" : "NO") << '\n';

    std::cout << "Overwrite EXT            : "
              << (capability.overwriteSupported ? "YES" : "NO") << '\n';

    std::cout << "\nRESULT : "
              << (capability.identifyAvailable ? "PASS" : "FAIL")
              << '\n';

    std::cout << "============================================================\n";

    return capability.identifyAvailable ? 0 : 1;
}