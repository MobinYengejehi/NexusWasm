#include "wasmtime/WasmtimeProbe.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    std::string ReadFile(const std::string& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            return {};
        }

        std::ostringstream buffer;
        buffer << stream.rdbuf();

        return buffer.str();
    }

    std::string Fixture(const char* name)
    {
        return std::string{ NEXUSWASM_TEST_FIXTURE_DIR } + "/" + name;
    }

    bool Require(const bool condition, const char* message)
    {
        if (condition)
        {
            return true;
        }

        std::cerr << "FAILED: " << message << std::endl;

        return false;
    }
}

int main()
{
    const std::string addWat = ReadFile(Fixture("add.wat"));
    if (!Require(!addWat.empty(), "could not read add.wat"))
    {
        return 1;
    }

    const auto addResult = nexus::detail::RunWasmtimeI32Binary(addWat, "add", 20, 22);
    if (!Require(addResult.success, addResult.message.c_str()))
    {
        return 2;
    }

    if (!Require(addResult.value == 42, "add(20, 22) must equal 42"))
    {
        return 3;
    }

    const auto invalidWat = nexus::detail::RunWasmtimeI32Binary("(module (func", "add", 20, 22);
    if (!Require(!invalidWat.success, "invalid WAT unexpectedly succeeded"))
    {
        return 4;
    }

    if (!Require(
        invalidWat.failure == nexus::detail::WasmtimeProbeFailure::WatParsing,
        "Invalid WAT returned wrong failure stage"
    ))
    {
        return 5;
    }

    const auto missingExport = nexus::detail::RunWasmtimeI32Binary(addWat, "does_not_exist", 20, 22);
    if (!Require(!missingExport.success, "missing export unexpectedly succeeded"))
    {
        return 6;
    }

    if (!Require(
        missingExport.failure == nexus::detail::WasmtimeProbeFailure::ExportNotFound,
        "missing export returned wrong failure stage"
    ))
    {
        return 7;
    }

    const std::string trapWat = ReadFile(Fixture("trap.wat"));
    if (!Require(!trapWat.empty(), "could not read trap.wat"))
    {
        return 8;
    }

    const auto trapped = nexus::detail::RunWasmtimeI32Binary(trapWat, "add", 20, 22);
    if (!Require(!trapped.success, "trap module unexpectedly succeeded"))
    {
        return 9;
    }

    if (!Require(
        trapped.failure == nexus::detail::WasmtimeProbeFailure::Trap,
        "trap returned wrong failure stage"
    ))
    {
        return 10;
    }

    const std::string importWat = ReadFile(Fixture("missing_import.wat"));
    if (!Require(!importWat.empty(), "could not read missing_import.wat"))
    {
        return 11;
    }

    const auto importFailure = nexus::detail::RunWasmtimeI32Binary(importWat, "add", 20, 22);
    if (!Require(!importFailure.success, "module with unresolved import unexpectedly instantiated"))
    {
        return 12;
    }

    if (!Require(
        importFailure.failure == nexus::detail::WasmtimeProbeFailure::Instantiation,
        "unresolved import returned wrong failure stage"
    ))
    {
        return 13;
    }

    std::cout << "Wasmtime integration OK: "
        << "add(20, 22)"
        << addResult.value
        << std::endl;

    return 0;
}
