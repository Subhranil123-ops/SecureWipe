#include "ConfidenceScorer.h"

#include <iostream>

int main()
{
    ConfidenceScorer scorer;

    EvidenceItem item;

    item.headerValid = true;
    item.footerValid = true;
    item.sizeValid = true;
    item.structureValid = true;
    item.decodable = true;

    scorer.calculate(item);

    std::cout << "Confidence Score: "
              << item.confidenceScore << std::endl;

    std::cout << "Confidence: "
              << item.getConfidenceString() << std::endl;

    std::cout << "Confidence Reasons:" << std::endl;

    for (const std::string& reason : item.confidenceReasons)
    {
        std::cout << "- " << reason << std::endl;
    }

    if (item.confidenceScore != 100)
    {
        std::cout << "ConfidenceScorer test FAILED." << std::endl;
        return 1;
    }

    if (item.confidence != ForensicConfidence::HIGH)
    {
        std::cout << "ConfidenceScorer test FAILED." << std::endl;
        return 1;
    }

    if (item.confidenceReasons.size() != 5)
    {
        std::cout << "ConfidenceScorer test FAILED." << std::endl;
        return 1;
    }

    std::cout << "ConfidenceScorer test PASSED." << std::endl;

    return 0;
}