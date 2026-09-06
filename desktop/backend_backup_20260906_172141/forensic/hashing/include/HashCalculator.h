#pragma once

#include <string>

class HashCalculator
{
public:
    bool calculateSha256(const std::string &filePath, std::string &hash) const;
};