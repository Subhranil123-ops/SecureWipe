#pragma once

#include "VerificationResult.h"

#include <Windows.h>
#include <cstdint>
#include <cstddef>

class HostOverwriteSanitizer
{
public:
    VerificationResult sanitize(
        HANDLE deviceHandle,
        std::uint64_t totalBytes);

private:
    bool getSectorSize(
        HANDLE deviceHandle,
        std::uint32_t& sectorSize);

    bool checkWritable(
        HANDLE deviceHandle,
        VerificationResult& result);

    bool overwrite(
        HANDLE deviceHandle,
        std::uint64_t totalBytes,
        std::uint32_t sectorSize,
        VerificationResult& result);

    VerificationResult verify(
        HANDLE deviceHandle,
        std::uint64_t totalBytes,
        std::uint32_t sectorSize);

    static constexpr std::size_t BUFFER_SIZE = 1024 * 1024;
    static constexpr std::size_t VERIFY_SIZE = 4096;
};