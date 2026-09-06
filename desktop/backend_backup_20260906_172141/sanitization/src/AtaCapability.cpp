#include "AtaCapability.h"

#include <Windows.h>
#include <Ntddscsi.h>
#include <cstring>
#include <iostream>

namespace
{
constexpr UCHAR ATA_IDENTIFY_DEVICE = 0xEC;

constexpr USHORT ATA_SECURITY_SUPPORTED = 0x0001;
constexpr USHORT ATA_SECURITY_ENABLED = 0x0002;
constexpr USHORT ATA_SECURITY_LOCKED = 0x0004;
constexpr USHORT ATA_SECURITY_FROZEN = 0x0008;
constexpr USHORT ATA_ENHANCED_ERASE_SUPPORTED = 0x0020;

// IDENTIFY DEVICE word 59
constexpr USHORT ATA_SANITIZE_OVERWRITE_SUPPORTED = 0x8000;
constexpr USHORT ATA_SANITIZE_BLOCK_ERASE_SUPPORTED = 0x4000;
constexpr USHORT ATA_SANITIZE_CRYPTO_SCRAMBLE_SUPPORTED = 0x2000;
constexpr USHORT ATA_SANITIZE_SUPPORTED = 0x1000;
}

AtaCapability detectAtaCapability(const StorageDevice& device)
{
    AtaCapability capability;

    if (device.getDeviceId().empty())
        return capability;

    HANDLE deviceHandle = CreateFileA(
        device.getDeviceId().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        std::cout
            << "ATA capability: failed to open device. Error: "
            << GetLastError()
            << '\n';

        return capability;
    }

    alignas(4) UCHAR identifyBuffer[512]{};

    ATA_PASS_THROUGH_DIRECT command{};

    command.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
    command.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
    command.DataTransferLength = sizeof(identifyBuffer);
    command.TimeOutValue = 10;
    command.DataBuffer = identifyBuffer;

    command.CurrentTaskFile[6] = ATA_IDENTIFY_DEVICE;

    DWORD returnedBytes = 0;

    const BOOL success = DeviceIoControl(
        deviceHandle,
        IOCTL_ATA_PASS_THROUGH_DIRECT,
        &command,
        sizeof(command),
        &command,
        sizeof(command),
        &returnedBytes,
        nullptr);

    DWORD errorCode = GetLastError();

    CloseHandle(deviceHandle);

    if (!success)
    {
        std::cout
            << "ATA IDENTIFY DEVICE failed. Error: "
            << errorCode
            << '\n';

        return capability;
    }

    capability.identifyAvailable = true;

    const auto* identify =
        reinterpret_cast<const USHORT*>(identifyBuffer);

    // --------------------------------------------------
    // ATA SECURITY STATUS - WORD 128
    // --------------------------------------------------

    const USHORT securityStatus = identify[128];

    capability.securitySupported =
        (securityStatus & ATA_SECURITY_SUPPORTED) != 0;

    capability.securityEnabled =
        (securityStatus & ATA_SECURITY_ENABLED) != 0;

    capability.securityLocked =
        (securityStatus & ATA_SECURITY_LOCKED) != 0;

    capability.securityFrozen =
        (securityStatus & ATA_SECURITY_FROZEN) != 0;

    capability.enhancedEraseSupported =
        (securityStatus & ATA_ENHANCED_ERASE_SUPPORTED) != 0;

    // --------------------------------------------------
    // ATA SANITIZE CAPABILITY - WORD 59
    // --------------------------------------------------

    const USHORT sanitizeCapabilities = identify[59];

    capability.sanitizeSupported =
        (sanitizeCapabilities & ATA_SANITIZE_SUPPORTED) != 0;

    capability.cryptoScrambleSupported =
        (sanitizeCapabilities &
         ATA_SANITIZE_CRYPTO_SCRAMBLE_SUPPORTED) != 0;

    capability.blockEraseSupported =
        (sanitizeCapabilities &
         ATA_SANITIZE_BLOCK_ERASE_SUPPORTED) != 0;

    capability.overwriteSupported =
        (sanitizeCapabilities &
         ATA_SANITIZE_OVERWRITE_SUPPORTED) != 0;

    // --------------------------------------------------
    // DISPLAY RESULT
    // --------------------------------------------------

    std::cout << "\nATA Capability:\n";

    std::cout
        << "  IDENTIFY available: "
        << capability.identifyAvailable
        << '\n';

    std::cout
        << "  Security supported: "
        << capability.securitySupported
        << '\n';

    std::cout
        << "  Enhanced erase supported: "
        << capability.enhancedEraseSupported
        << '\n';

    std::cout
        << "  Security enabled: "
        << capability.securityEnabled
        << '\n';

    std::cout
        << "  Security locked: "
        << capability.securityLocked
        << '\n';

    std::cout
        << "  Security frozen: "
        << capability.securityFrozen
        << '\n';

    std::cout << "\nATA Sanitize Capability:\n";

    std::cout
        << "  Sanitize supported: "
        << capability.sanitizeSupported
        << '\n';

    std::cout
        << "  Crypto Scramble EXT: "
        << capability.cryptoScrambleSupported
        << '\n';

    std::cout
        << "  Block Erase EXT: "
        << capability.blockEraseSupported
        << '\n';

    std::cout
        << "  Overwrite EXT: "
        << capability.overwriteSupported
        << '\n';

    return capability;
}