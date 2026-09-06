#include <iostream>

#include "WindowsBootChecker.h"


int main()
{
    std::cout
        << "========================================\n";

    std::cout
        << "      WindowsBootChecker Test\n";

    std::cout
        << "========================================\n\n";


    WindowsBootChecker checker;


    BootInfo result =
        checker.checkBootInfo();


    std::cout
        << "\n----------------------------------------\n";

    std::cout
        << "WindowsBootChecker Result\n";

    std::cout
        << "----------------------------------------\n";


   
    if (result.isValid())
    {
        std::cout
            << "Status: PASSED\n";
    }
    else
    {
        std::cout
            << "Status: FAILED\n";
    }


    std::cout
        << "----------------------------------------\n";


    return result.isValid() ? 0 : 1;
}