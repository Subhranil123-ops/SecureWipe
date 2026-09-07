#include "CertificateVerifier.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace
{
    std::filesystem::path defaultEvidenceDirectory()
    {
#ifdef _WIN32
        const char* localAppData =
            std::getenv("LOCALAPPDATA");

        if (localAppData != nullptr &&
            localAppData[0] != '\0')
        {
            return std::filesystem::path(
                       localAppData) /
                   "SecureWipe" /
                   "evidence";
        }
#endif

        const char* home =
            std::getenv("HOME");

        if (home != nullptr &&
            home[0] != '\0')
        {
            return std::filesystem::path(
                       home) /
                   ".securewipe" /
                   "evidence";
        }

        return std::filesystem::current_path() /
               "evidence";
    }

    std::filesystem::path findNewestCertificate()
    {
        const std::filesystem::path directory =
            defaultEvidenceDirectory() /
            "certificates";

        if (!std::filesystem::exists(
                directory))
        {
            return {};
        }

        std::filesystem::path newest;
        std::filesystem::file_time_type newestTime{};

        for (const auto& entry :
             std::filesystem::directory_iterator(
                 directory))
        {
            if (!entry.is_regular_file())
                continue;

            if (entry.path().extension() !=
                ".json")
            {
                continue;
            }

            const auto writeTime =
                std::filesystem::last_write_time(
                    entry.path());

            if (newest.empty() ||
                writeTime > newestTime)
            {
                newest = entry.path();
                newestTime = writeTime;
            }
        }

        return newest;
    }
}

int main(int argc, char* argv[])
{
    std::cout
        << "============================================================\n"
        << "            SECUREWIPE CERTIFICATE VERIFIER\n"
        << "            NON-DESTRUCTIVE EVIDENCE CHECK\n"
        << "============================================================\n\n";

    const std::filesystem::path certificatePath =
        argc > 1
            ? std::filesystem::path(argv[1])
            : findNewestCertificate();

    if (certificatePath.empty())
    {
        std::cout
            << "[FAIL] No certificate file was supplied or found.\n";
        return 1;
    }

    std::cout
        << "Certificate:\n"
        << certificatePath.string()
        << "\n\n";

    CertificateVerifier verifier;

    const CertificateVerificationResult result =
        verifier.verify(certificatePath);

    std::cout
        << "Certificate ID : "
        << result.certificateId
        << '\n'
        << "Operation ID   : "
        << result.operationId
        << '\n'
        << "Device ID      : "
        << result.deviceId
        << '\n'
        << "Serial Number  : "
        << result.serialNumber
        << '\n'
        << "Capacity       : "
        << result.capacityBytes
        << " bytes\n\n";

    std::cout
        << "Required fields : "
        << (result.requiredFieldsPresent
                ? "PASS"
                : "FAIL")
        << '\n'
        << "Stored hash     : "
        << result.storedHash
        << '\n'
        << "Calculated hash : "
        << result.calculatedHash
        << '\n'
        << "Hash match      : "
        << (result.hashMatched
                ? "PASS"
                : "FAIL")
        << '\n'
        << "Operation evidence: "
        << (result.successfulSanitizationEvidence
                ? "PASS"
                : "FAIL")
        << '\n';

    std::cout
        << "\nResult:\n"
        << result.message
        << '\n';

    if (!result.isSuccess())
    {
        std::cout
            << "\n[FAIL] Certificate verification failed.\n";

        return 1;
    }

    std::cout
        << "\n[PASS] Certificate verification passed.\n";

    return 0;
}