#pragma once

#include "StorageDevice.h"

struct AtaCapability
{
    bool identifyAvailable = false;

    // Security
    bool securitySupported = false;
    bool enhancedEraseSupported = false;
    bool securityEnabled = false;
    bool securityLocked = false;
    bool securityFrozen = false;

    // ATA Sanitize
    bool sanitizeSupported = false;
    bool cryptoScrambleSupported = false;
    bool blockEraseSupported = false;
    bool overwriteSupported = false;
};

AtaCapability detectAtaCapability(const StorageDevice &device);