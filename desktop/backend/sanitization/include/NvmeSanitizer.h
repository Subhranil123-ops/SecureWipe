#pragma once

#include <Windows.h>
#include <cstdint>

#include "SanitizationResult.h"

enum class NvmeSanitizeMethod
{
    BlockErase,
    CryptoErase,
    Overwrite
};

bool executeNvmeSanitize(
    HANDLE deviceHandle,
    NvmeSanitizeMethod method,
    std::uint64_t totalBytes,
    const SecureWipe::SanitizationProgressCallback& progressCallback = {});