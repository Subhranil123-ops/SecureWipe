#pragma once
#include <string>
#include <vector>

#include "DeviceIdentity.h"

struct SafetyCheckResult
{
    std::string checkName;
    bool passed;
    std::string message;
};

struct SafetyResult
{
    bool isOverallSafe;
    std::string decision;
    std::string summary;
    
    std::vector<SafetyCheckResult> checks;
};