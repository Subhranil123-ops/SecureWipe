#pragma once

#include "StorageDevice.h"

enum class NativeSanitizeSupport
{
    UNKNOWN,
    NOT_SUPPORTED,
    SUPPORTED
};

struct SanitizationCapability
{
    // USB

    bool isUsbDevice = false;
    bool scsiPathAvailable = false;
    bool storagePropertyQueryAvailable = false;
    NativeSanitizeSupport nativeSanitizeSupported =
        NativeSanitizeSupport::UNKNOWN;

    // NVMe sanitization capabilities

    bool nvmeIdentifyAvailable = false;
    bool nvmeBlockEraseSupported = false;
    bool nvmeCryptoEraseSupported = false;
    bool nvmeOverwriteSupported = false;

    // SATA HDD

    bool ataIdentifyAvailable = false;
    bool ataSecuritySupported = false;
    bool ataEnhancedEraseSupported = false;
    bool ataSecurityEnabled = false;
    bool ataSecurityLocked = false;
    bool ataSecurityFrozen = false;

    // ATA Sanitize
    bool atasanitizeSupported = false;
    bool atacryptoScrambleSupported = false;
    bool atablockEraseSupported = false;
    bool ataoverwriteSupported = false;
};

SanitizationCapability detectSanitizationCapability(
    const StorageDevice &device);