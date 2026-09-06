#include "ConfidenceScorer.h"

void ConfidenceScorer::calculate(EvidenceItem &item) const
{
    item.confidenceScore = 0;
    item.confidence = ForensicConfidence::UNKNOWN;
    item.confidenceReasons.clear();

    if (item.headerValid)
    {
        item.confidenceScore += 20;
        item.confidenceReasons.push_back("Valid JPEG header.");
    }

    if (item.footerValid)
    {
        item.confidenceScore += 20;
        item.confidenceReasons.push_back("Valid JPEG footer.");
    }

    if (item.sizeValid)
    {
        item.confidenceScore += 15;
        item.confidenceReasons.push_back("Valid artifact size.");
    }

    if (item.structureValid)
    {
        item.confidenceScore += 20;
        item.confidenceReasons.push_back("Valid JPEG structure.");
    }

    if (item.decodable)
    {
        item.confidenceScore += 25;
        item.confidenceReasons.push_back("JPEG is successfully decodable.");
    }

    if (item.confidenceScore >= 80)
    {
        item.confidence = ForensicConfidence::HIGH;
    }
    else if (item.confidenceScore >= 50)
    {
        item.confidence = ForensicConfidence::MEDIUM;
    }
    else if (item.confidenceScore > 0)
    {
        item.confidence = ForensicConfidence::LOW;
    }
    else
    {
        item.confidence = ForensicConfidence::UNKNOWN;
    }
};