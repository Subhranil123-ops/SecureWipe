#pragma once

#include <Windows.h>
#include "VerificationResult.h"

enum class AtaSanitizeMethod
{
    CryptoScramble,
    BlockErase,
    Overwrite
};

VerificationResult executeAtaSanitize(HANDLE deviceHandle, AtaSanitizeMethod method);