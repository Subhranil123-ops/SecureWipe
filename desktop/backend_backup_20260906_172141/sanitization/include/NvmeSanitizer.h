#pragma once

#include <Windows.h>

enum class NvmeSanitizeMethod
{
    BlockErase,
    CryptoErase,
    Overwrite
};

bool executeNvmeSanitize(HANDLE deviceHandle, NvmeSanitizeMethod method);