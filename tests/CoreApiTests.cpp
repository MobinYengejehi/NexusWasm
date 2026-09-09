#include <nexuswasm/Runtime.hpp>

#include <cstdint>
#include <iostream>
#include <utility>

namespace
{
    constexpr std::uint8_t AddModule[] = {
        0x00, 0x61, 0x73, 0x6D,
        0x01, 0x00, 0x00, 0x00,

        0x01, 0x07,
        0x01,
        0x60,
        0x02,
        0x7F, 0x7F,
        0x01,
        0x7F,

        0x03, 0x02,
        0x01, 0x00,

        0x07, 0x07,
        0x01,
        0x03,
        0x61, 0x64, 0x64,
        0x00, 0x00,

        0x0A, 0x09,
        0x01,
        0x07,
        0x00,
        0x20, 0x00,
        0x20, 0x01,
        0x6A,
        0x0B
    };

    constexpr std::uint8_t TrapModule[] = {
        0x00, 0x61, 0x73, 0x6D,
        0x01, 0x00, 0x00, 0x00,

        0x01, 0x07,
        0x01,
        0x60,
        0x02,
        0x7F, 0x7F,
        0x01,
        0x7F,

        0x03, 0x02,
        0x01, 0x00,

        0x07, 0x07,
        0x01,
        0x03,
        0x61, 0x64, 0x64,
        0x00, 0x00,

        0x0A, 0x05,
        0x01,
        0x03,
        0x00,
        0x00,
        0x0B
    };

    bool Require(
        const bool condition,
        const char* message
    )
    {
        if(condition)
        {
            return true;
        }

        std::cerr
            << "FAILED: "
            << message
            << '\n';

        return false;
    }

    nexus::Result<nexus::Instance> CreateDetachedInstance()
    {
        auto runtimeResult = nexus::Runtime::Create();

        if(!runtimeResult)
        {
            return runtimeResult.GetError();
        }

        auto runtime = std::move(runtimeResult).Value();

        auto moduleResult = runtime.Compile(AddModule, sizeof(AddModule));
        if(!moduleResult)
        {
            return moduleResult.GetError();
        }

        auto module = std::move(moduleResult).Value();

        return runtime.Instantiate(module);
    }
}

int main()
{
    // --------------------------------------------------------
    // Runtime
    // --------------------------------------------------------

    auto runtimeResult = nexus::Runtime::Create();

    if(!Require(
        static_cast<bool>(runtimeResult),
        "Runtime creation failed"
    ))
    {
        return 1;
    }

    auto runtime = std::move(runtimeResult).Value();

    // --------------------------------------------------------
    // Compile
    // --------------------------------------------------------

    auto moduleResult = runtime.Compile(AddModule, sizeof(AddModule));
    if(!Require(
        static_cast<bool>(moduleResult),
        "Module compilation failed"
    ))
    {
        return 2;
    }

    auto module = std::move(moduleResult).Value();

    // --------------------------------------------------------
    // Successful call
    // --------------------------------------------------------

    auto instanceResult = runtime.Instantiate(module);
    if(!Require(
        static_cast<bool>(instanceResult),
        "Instantiation failed"
    ))
    {
        return 3;
    }

    auto instance = std::move(instanceResult).Value();

    auto addResult = instance.Call<std::int32_t>(
        "add",
        std::int32_t{20},
        std::int32_t{22}
    );
    if(!Require(
        static_cast<bool>(addResult),
        "add call failed"
    ))
    {
        return 4;
    }
    if(!Require(
        addResult.Value() == 42,
        "add(20, 22) != 42"
    ))
    {
        return 5;
    }

    // --------------------------------------------------------
    // Missing export
    // --------------------------------------------------------

    auto missingExport = instance.Call<std::int32_t>(
        "does_not_exist",
        std::int32_t{20},
        std::int32_t{22}
    );
    if(!Require(
        !missingExport,
        "Missing export unexpectedly succeeded"
    ))
    {
        return 6;
    }
    if(!Require(
        missingExport.GetError().Code() == nexus::ErrorCode::ExportNotFound,
        "Wrong missing-export error"
    ))
    {
        return 7;
    }

    // --------------------------------------------------------
    // Wrong result signature
    // --------------------------------------------------------

    auto wrongResult = instance.Call<std::int64_t>(
        "add",
        std::int32_t{20},
        std::int32_t{22}
    );
    if(!Require(
        !wrongResult,
        "Wrong signature unexpectedly succeeded"
    ))
    {
        return 8;
    }
    if(!Require(
        wrongResult.GetError().Code() == nexus::ErrorCode::SignatureMismatch,
        "Wrong signature error code"
    ))
    {
        return 9;
    }

    // --------------------------------------------------------
    // Wrong argument signature
    // --------------------------------------------------------

    auto wrongArgument = instance.Call<std::int32_t>(
        "add",
        std::int64_t{20},
        std::int32_t{22}
    );
    if(!Require(
        !wrongArgument,
        "Wrong argument type unexpectedly succeeded"
    ))
    {
        return 10;
    }

    // --------------------------------------------------------
    // Multiple instances
    // --------------------------------------------------------

    auto instanceTwoResult = runtime.Instantiate(module);
    if(!Require(
        static_cast<bool>(instanceTwoResult),
        "Second instance failed"
    ))
    {
        return 11;
    }

    auto instanceTwo = std::move(instanceTwoResult).Value();

    auto secondCall = instanceTwo.Call<std::int32_t>(
        "add",
        std::int32_t{40},
        std::int32_t{2}
    );
    if(!Require(
        secondCall && secondCall.Value() == 42,
        "Second instance call failed"
    ))
    {
        return 12;
    }

    // --------------------------------------------------------
    // CompiledModule copy lifetime
    // --------------------------------------------------------

    nexus::CompiledModule moduleCopy = module;

    auto copyInstanceResult = runtime.Instantiate(moduleCopy);
    if(!Require(
        static_cast<bool>(copyInstanceResult),
        "Copied module failed to instantiate"
    ))
    {
        return 13;
    }

    // --------------------------------------------------------
    // Invalid wasm
    // --------------------------------------------------------

    constexpr std::uint8_t invalidWasm[] =
    {
        0x01,
        0x02,
        0x03,
        0x04
    };


    auto invalidResult = runtime.Compile(invalidWasm, sizeof(invalidWasm));
    if(!Require(
        !invalidResult,
        "Invalid wasm unexpectedly compiled"
    ))
    {
        return 14;
    }
    if(!Require(
        invalidResult.GetError().Code() == nexus::ErrorCode::CompilationFailed,
        "Wrong invalid-wasm error"
    ))
    {
        return 15;
    }

    // --------------------------------------------------------
    // Trap
    // --------------------------------------------------------

    auto trapModuleResult = runtime.Compile(TrapModule, sizeof(TrapModule));
    if(!Require(
        static_cast<bool>(trapModuleResult),
        "Trap module failed to compile"
    ))
    {
        return 16;
    }

    auto trapModule = std::move(trapModuleResult).Value();

    auto trapInstanceResult = runtime.Instantiate(trapModule);
    if(!Require(
        static_cast<bool>(trapInstanceResult),
        "Trap module failed to instantiate"
    ))
    {
        return 17;
    }

    auto trapInstance = std::move(trapInstanceResult).Value();

    auto trapResult = trapInstance.Call<std::int32_t>(
        "add",
        std::int32_t{20},
        std::int32_t{22}
    );
    if(!Require(
        !trapResult,
        "Trap unexpectedly succeeded"
    ))
    {
        return 18;
    }

    if(!Require(
        trapResult.GetError().Code() == nexus::ErrorCode::Trap,
        "Trap returned wrong error code"
    ))
    {
        return 19;
    }

    // --------------------------------------------------------
    // Runtime destruction
    // --------------------------------------------------------

    auto detachedResult = CreateDetachedInstance();
    if(!Require(
        static_cast<bool>(detachedResult),
        "Detached instance creation failed"
    ))
    {
        return 20;
    }

    auto detached = std::move(detachedResult).Value();

    auto detachedCall = detached.Call<std::int32_t>(
        "add",
        std::int32_t{20},
        std::int32_t{22}
    );
    if(!Require(
        detachedCall && detachedCall.Value() == 42,
        "Instance failed after Runtime destruction"
    ))
    {
        return 21;
    }

    // --------------------------------------------------------
    // Runtime mismatch
    // --------------------------------------------------------

    auto runtimeTwoResult = nexus::Runtime::Create();
    if(!Require(
        static_cast<bool>(runtimeTwoResult),
        "Second Runtime creation failed"
    ))
    {
        return 22;
    }

    auto runtimeTwo = std::move(runtimeTwoResult).Value();

    auto mismatched = runtimeTwo.Instantiate(module);
    if(!Require(
        !mismatched,
        "Cross-runtime module unexpectedly instantiated"
    ))
    {
        return 23;
    }


    if(!Require(
        mismatched.GetError().Code() == nexus::ErrorCode::RuntimeMismatch,
        "Wrong RuntimeMismatch error"
    ))
    {
        return 24;
    }

    std::cout
        << "NexusWasm Core API OK\n"
        << "add(20, 22) = "
        << addResult.Value()
        << '\n';

    return 0;
}
