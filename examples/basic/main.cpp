#include <nexuswasm/Version.hpp>

#include <iostream>

int main()
{
    std::cout
        << "NexusWasm "
        << nexus::GetVersionString()
        << '\n';

    return 0;
}
