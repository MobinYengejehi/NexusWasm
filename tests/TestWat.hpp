#pragma once

#ifndef NEXUSWASM_TEST_WAT_HEADER
#define NEXUSWASM_TEST_WAT_HEADER

#include <cstdint>
#include <string_view>
#include <vector>

namespace nexus::test
{
    [[nodiscard]]
    std::vector<std::uint8_t> WatFileToWasm(std::string_view fileName);
}

#endif

