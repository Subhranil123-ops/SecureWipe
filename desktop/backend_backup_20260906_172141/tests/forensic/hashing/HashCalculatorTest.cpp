#include "HashCalculator.h"

#include <iostream>
#include <string>

int main()
{
    HashCalculator calculator;

    std::string hash;

    bool result = calculator.calculateSha256(
        "dummy_file.txt",
        hash
    );

    const std::string expectedHash =
        "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";

    if (!result)
    {
        std::cout << "HashCalculator test FAILED." << std::endl;
        std::cout << "Hash calculation failed." << std::endl;
        return 1;
    }

    std::cout << "Actual SHA-256:   " << hash << std::endl;
    std::cout << "Expected SHA-256: " << expectedHash << std::endl;

    if (hash != expectedHash)
    {
        std::cout << "HashCalculator test FAILED." << std::endl;
        return 1;
    }

    std::cout << "HashCalculator test PASSED." << std::endl;
    return 0;
}