#pragma once

#include "EvidenceItem.h"

class ConfidenceScorer
{
public:
    void calculate(EvidenceItem &item) const;
};