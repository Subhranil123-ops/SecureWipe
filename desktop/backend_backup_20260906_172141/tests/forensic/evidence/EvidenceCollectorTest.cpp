#include "EvidenceCollector.h"

#include <iostream>
#include <string>
#include <vector>

int main()
{
    EvidenceCollector collector;

    std::vector<EvidenceItem> evidence =
        collector.collect("test_source.jpeg");

    if (evidence.empty())
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "No valid evidence recovered." << std::endl;
        return 1;
    }

    const EvidenceItem& item = evidence[0];

    std::cout << "Artifact ID: "
              << item.artifactId << std::endl;

    std::cout << "Recovered path: "
              << item.recoveredPath << std::endl;

    std::cout << "Recovered: "
              << (item.recovered ? "YES" : "NO") << std::endl;

    std::cout << "Validated: "
              << (item.validated ? "YES" : "NO") << std::endl;

    std::cout << "SHA-256: "
              << item.sha256 << std::endl;

    std::cout << "Confidence Score: "
              << item.confidenceScore << std::endl;

    std::cout << "Confidence: "
              << item.getConfidenceString() << std::endl;

    if (!item.recovered)
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "Artifact was not marked as recovered." << std::endl;
        return 1;
    }

    if (!item.validated)
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "Evidence was not validated." << std::endl;
        return 1;
    }

    if (item.sha256.empty())
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "SHA-256 was not generated." << std::endl;
        return 1;
    }

    if (item.sha256.length() != 64)
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "Invalid SHA-256 length: "
                  << item.sha256.length() << std::endl;
        return 1;
    }

    if (item.confidenceScore != 100)
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "Unexpected confidence score: "
                  << item.confidenceScore << std::endl;
        return 1;
    }

    if (item.confidence != ForensicConfidence::HIGH)
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "Confidence level is not HIGH." << std::endl;
        return 1;
    }

    if (item.confidenceReasons.size() != 5)
    {
        std::cout << "EvidenceCollector test FAILED." << std::endl;
        std::cout << "Expected 5 confidence reasons, got "
                  << item.confidenceReasons.size() << std::endl;
        return 1;
    }

    std::cout << "EvidenceCollector test PASSED." << std::endl;

    return 0;
}