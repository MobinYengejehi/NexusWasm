#include <nexuswasm/Version.hpp>

#include <iostream>
#include <string_view>

int main()
{
    const nexus::Version version = nexus::GetVersion();

    if(version.major != NEXUSWASM_EXPECTED_VERSION_MAJOR)
    {
        std::cerr << "Unexpected major version\n";
        return 1;
    }

    if(version.minor != NEXUSWASM_EXPECTED_VERSION_MINOR)
    {
        std::cerr << "Unexpected minor version\n";
        return 2;
    }

    if(version.patch != NEXUSWASM_EXPECTED_VERSION_PATCH)
    {
        std::cerr << "Unexpected patch version\n";
        return 3;
    }

    const std::string_view versionString{
        nexus::GetVersionString()
    };

    if(versionString != NEXUSWASM_EXPECTED_VERSION)
    {
        std::cerr
            << "Unexpected version string: "
            << versionString
            << '\n';

        return 4;
    }

    return 0;
}
