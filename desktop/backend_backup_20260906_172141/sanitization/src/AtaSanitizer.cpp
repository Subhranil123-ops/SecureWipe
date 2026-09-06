#include "AtaSanitizer.h"
#include <Windows.h>
#include <Ntddscsi.h>
#include <iostream>
#include "VerificationResult.h"

namespace
{
    constexpr UCHAR ATA_SANITIZE_DEVICE = 0xB4;

    constexpr USHORT ATA_SANITIZE_CRYPTO_SCRAMBLE = 0x0011;
    constexpr USHORT ATA_SANITIZE_BLOCK_ERASE = 0x0012;
    constexpr USHORT ATA_SANITIZE_OVERWRITE = 0x0014;

    constexpr UCHAR ATA_STATUS_ERR = 0x01;
    constexpr UCHAR ATA_STATUS_BSY = 0x80;
}

VerificationResult checkSanitizeStatus(HANDLE deviceHandle)
{
    VerificationResult result;

    result.performed = true;

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.message =
            "ATA SANITIZE verification failed: invalid device handle.";
        return result;
    }

    ATA_PASS_THROUGH_DIRECT command{};

    command.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
    command.AtaFlags = ATA_FLAGS_48BIT_COMMAND |
                       ATA_FLAGS_DRDY_REQUIRED;
    command.DataTransferLength = 0;
    command.TimeOutValue = 10;
    command.DataBuffer = nullptr;

    // SANITIZE STATUS EXT
    command.CurrentTaskFile[0] = 0x00;
    command.CurrentTaskFile[1] = 0x00;
    command.CurrentTaskFile[6] = ATA_SANITIZE_DEVICE;

    DWORD returnedBytes = 0;

    BOOL success = DeviceIoControl(
        deviceHandle,
        IOCTL_ATA_PASS_THROUGH_DIRECT,
        &command,
        sizeof(command),
        &command,
        sizeof(command),
        &returnedBytes,
        nullptr);

    if (!success)
    {
        std::cout
            << "ATA SANITIZE STATUS failed. Windows error: "
            << GetLastError() << '\n';

        result.passed = false;
        result.message =
            "ATA SANITIZE status command failed.";

        return result;
    }

    const UCHAR status = command.CurrentTaskFile[6];

    if (status & ATA_STATUS_ERR)
    {
        std::cout << "ATA SANITIZE STATUS returned error.\n";

        result.passed = false;
        result.message =
            "ATA SANITIZE status reported a device error.";

        return result;
    }

    if (status & ATA_STATUS_BSY)
    {
        std::cout
            << "ATA SANITIZE operation is still running.\n";

        result.passed = false;
        result.message =
            "ATA SANITIZE operation is still running.";

        return result;
    }

    result.passed = true;
    result.message =
        "ATA SANITIZE status verification passed.";

    std::cout << result.message << '\n';

    return result;
}

bool sendSanitizeCommand(HANDLE deviceHandle, AtaSanitizeMethod method)
{
    if (deviceHandle == INVALID_HANDLE_VALUE)
        return false;

    USHORT feature = 0;

    switch (method)
    {
    case AtaSanitizeMethod::CryptoScramble:
        feature = ATA_SANITIZE_CRYPTO_SCRAMBLE;
        break;

    case AtaSanitizeMethod::BlockErase:
        feature = ATA_SANITIZE_BLOCK_ERASE;
        break;

    case AtaSanitizeMethod::Overwrite:
        feature = ATA_SANITIZE_OVERWRITE;
        break;

    default:
        return false;
    }

    ATA_PASS_THROUGH_DIRECT command{};

    command.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
    command.AtaFlags = ATA_FLAGS_48BIT_COMMAND |
                       ATA_FLAGS_DRDY_REQUIRED;
    command.DataTransferLength = 0;
    command.TimeOutValue = 10;
    command.DataBuffer = nullptr;

    command.CurrentTaskFile[0] =
        static_cast<UCHAR>(feature & 0xFF);

    command.PreviousTaskFile[0] =
        static_cast<UCHAR>((feature >> 8) & 0xFF);

    command.CurrentTaskFile[6] =
        ATA_SANITIZE_DEVICE;

    DWORD returnedBytes = 0;

    BOOL success = DeviceIoControl(
        deviceHandle,
        IOCTL_ATA_PASS_THROUGH_DIRECT,
        &command,
        sizeof(command),
        &command,
        sizeof(command),
        &returnedBytes,
        nullptr);

    if (!success)
    {
        std::cout
            << "ATA SANITIZE command failed. Windows error: "
            << GetLastError() << '\n';

        return false;
    }

    const UCHAR status = command.CurrentTaskFile[6];

    if (status & ATA_STATUS_ERR)
    {
        std::cout << "ATA SANITIZE returned device error.\n";
        return false;
    }

    std::cout << "ATA SANITIZE command accepted.\n";
    return true;
}

VerificationResult executeAtaSanitize(
    HANDLE deviceHandle,
    AtaSanitizeMethod method)
{
    VerificationResult result;

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        result.message =
            "ATA SANITIZE failed: invalid device handle.";
        return result;
    }

    std::cout << "\nStarting ATA SANITIZE...\n";

    if (!sendSanitizeCommand(deviceHandle, method))
    {
        result.message =
            "ATA SANITIZE command execution failed.";
        return result;
    }

    std::cout
        << "ATA SANITIZE command sent successfully.\n";

    return checkSanitizeStatus(deviceHandle);
}