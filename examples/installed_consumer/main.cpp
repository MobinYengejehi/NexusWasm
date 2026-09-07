#include <nexuswasm/Version.hpp>

#include <iostream>

int main()
{
    const nexus::Version version = nexus::GetVersion();

    std::cout
        << "Found installed NexusWasm "
        << version.major
        << '.'
        << version.minor
        << '.'
        << version.patch
        << '\n';

    return 0;
}
