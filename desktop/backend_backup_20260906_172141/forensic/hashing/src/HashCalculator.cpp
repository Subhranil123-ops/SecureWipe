#include "HashCalculator.h"

#include <Windows.h>
#include <bcrypt.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>

bool HashCalculator::calculateSha256(const std::string& filePath, std::string& hash) const
{
    hash.clear();

    BCRYPT_ALG_HANDLE algorithmHandle = nullptr;

    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &algorithmHandle,
        BCRYPT_SHA256_ALGORITHM,
        nullptr,
        0
    );

    if (status < 0)
    {
        return false;
    }

    DWORD hashObjectSize = 0;
    DWORD resultSize = 0;

    status = BCryptGetProperty(
        algorithmHandle,
        BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&hashObjectSize),
        sizeof(hashObjectSize),
        &resultSize,
        0
    );

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return false;
    }

    DWORD hashSize = 0;

    status = BCryptGetProperty(
        algorithmHandle,
        BCRYPT_HASH_LENGTH,
        reinterpret_cast<PUCHAR>(&hashSize),
        sizeof(hashSize),
        &resultSize,
        0
    );

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return false;
    }

    std::vector<unsigned char> hashObject(hashObjectSize);

    BCRYPT_HASH_HANDLE hashHandle = nullptr;

    status = BCryptCreateHash(
        algorithmHandle,
        &hashHandle,
        hashObject.data(),
        hashObjectSize,
        nullptr,
        0,
        0
    );

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return false;
    }

    std::ifstream file(filePath, std::ios::binary);

    if (!file)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return false;
    }

    const std::size_t bufferSize = 4 * 1024 * 1024;
    std::vector<unsigned char> buffer(bufferSize);

    while (file)
    {
        file.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );

        std::streamsize bytesRead = file.gcount();

        if (bytesRead > 0)
        {
            status = BCryptHashData(
                hashHandle,
                buffer.data(),
                static_cast<ULONG>(bytesRead),
                0
            );

            if (status < 0)
            {
                BCryptDestroyHash(hashHandle);
                BCryptCloseAlgorithmProvider(algorithmHandle, 0);
                return false;
            }
        }
    }

    if (file.bad())
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return false;
    }

    std::vector<unsigned char> hashBytes(hashSize);

    status = BCryptFinishHash(
        hashHandle,
        hashBytes.data(),
        hashSize,
        0
    );

    if (status < 0)
    {
        BCryptDestroyHash(hashHandle);
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return false;
    }

    std::ostringstream hashStream;

    for (unsigned char byte : hashBytes)
    {
        hashStream << std::hex
                   << std::setw(2)
                   << std::setfill('0')
                   << static_cast<int>(byte);
    }

    hash = hashStream.str();

    BCryptDestroyHash(hashHandle);
    BCryptCloseAlgorithmProvider(algorithmHandle, 0);

    return true;
}