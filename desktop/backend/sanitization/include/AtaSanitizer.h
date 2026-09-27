#pragma once

#include <Windows.h>

#include "VerificationResult.h"
#include "SanitizationResult.h"

enum class AtaSanitizeMethod
{
    CryptoScramble,
    BlockErase,
    Overwrite
};

VerificationResult executeAtaSanitize(
    HANDLE deviceHandle,
    AtaSanitizeMethod method,
    const SecureWipe::SanitizationProgressCallback& progressCallback = {});