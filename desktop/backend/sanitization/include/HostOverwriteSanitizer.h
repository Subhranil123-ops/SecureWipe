#pragma once

#include "VerificationResult.h"
#include "SanitizationResult.h"

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class HostOverwriteSanitizer
{
public:
    VerificationResult sanitize(
        HANDLE deviceHandle,
        std::uint64_t totalBytes,
        const SecureWipe::SanitizationProgressCallback& progressCallback = {});

private:
    bool getSectorSize(
        HANDLE deviceHandle,
        std::uint32_t& sectorSize);

    bool checkWritable(
        HANDLE deviceHandle,
        VerificationResult& result);

    bool lockTargetVolumes(
        HANDLE deviceHandle,
        std::vector<HANDLE>& lockedVolumes,
        VerificationResult& result);

    void unlockTargetVolumes(
        std::vector<HANDLE>& lockedVolumes);

    bool overwrite(
        HANDLE deviceHandle,
        std::uint64_t totalBytes,
        std::uint32_t sectorSize,
        VerificationResult& result,
        const SecureWipe::SanitizationProgressCallback& progressCallback);

    VerificationResult verify(
        HANDLE deviceHandle,
        std::uint64_t totalBytes,
        std::uint32_t sectorSize);

    static constexpr std::size_t BUFFER_SIZE =
        1024 * 1024;

    static constexpr std::size_t VERIFY_SIZE =
        4096;
};