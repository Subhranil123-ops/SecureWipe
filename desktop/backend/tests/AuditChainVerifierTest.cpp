#include "AuditChainVerifier.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
    std::filesystem::path defaultAuditPath()
    {
#ifdef _WIN32
        const char* localAppData =
            std::getenv("LOCALAPPDATA");

        if (localAppData != nullptr &&
            localAppData[0] != '\0')
        {
            return std::filesystem::path(localAppData) /
                   "SecureWipe" /
                   "evidence" /
                   "audit" /
                   "sanitization_audit.jsonl";
        }
#endif

        const char* home =
            std::getenv("HOME");

        if (home != nullptr &&
            home[0] != '\0')
        {
            return std::filesystem::path(home) /
                   ".securewipe" /
                   "evidence" /
                   "audit" /
                   "sanitization_audit.jsonl";
        }

        return std::filesystem::current_path() /
               "evidence" /
               "audit" /
               "sanitization_audit.jsonl";
    }
}

int main(int argc, char* argv[])
{
    std::cout
        << "============================================================\n"
        << "          SECUREWIPE AUDIT CHAIN VERIFIER\n"
        << "          NON-DESTRUCTIVE EVIDENCE VERIFICATION\n"
        << "============================================================\n\n";

    const std::filesystem::path auditPath =
        argc > 1
            ? std::filesystem::path(argv[1])
            : defaultAuditPath();

    std::cout
        << "Audit log:\n"
        << auditPath.string()
        << "\n\n";

    AuditChainVerifier verifier;

    const AuditChainVerificationResult result =
        verifier.verify(auditPath);

    std::cout
        << "Events found       : "
        << result.eventCount
        << '\n'
        << "Events verified    : "
        << result.verifiedEventCount
        << '\n';

    if (!result.firstEventPreviousHash.empty())
    {
        std::cout
            << "First previous hash: "
            << result.firstEventPreviousHash
            << '\n';
    }

    std::cout
        << "\nResult:\n"
        << result.message
        << "\n";

    if (!result.valid)
    {
        if (result.failedEventIndex != 0)
        {
            std::cout
                << "Failed event line : "
                << result.failedEventIndex
                << '\n';
        }

        if (!result.expectedHash.empty())
        {
            std::cout
                << "Expected hash      : "
                << result.expectedHash
                << '\n';
        }

        if (!result.actualHash.empty())
        {
            std::cout
                << "Stored/actual hash : "
                << result.actualHash
                << '\n';
        }

        std::cout
            << "\n[FAIL] Audit chain verification failed.\n";

        return 1;
    }

    std::cout
        << "\n[PASS] Audit chain verification passed.\n";

    return 0;
}