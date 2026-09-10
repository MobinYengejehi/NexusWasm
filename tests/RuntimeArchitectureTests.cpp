#include <nexuswasm/Error.hpp>
#include <nexuswasm/Instance.hpp>
#include <nexuswasm/Module.hpp>
#include <nexuswasm/Program.hpp>
#include <nexuswasm/Realm.hpp>
#include <nexuswasm/Runtime.hpp>

#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>

namespace
{

// ------------------------------------------------------------
// WebAssembly:
//
// (module
//   (func (export "add")
//     (param i32 i32)
//     (result i32)
//
//     local.get 0
//     local.get 1
//     i32.add
//   )
// )
//
// ------------------------------------------------------------

constexpr std::uint8_t AddModule[] =
{
    // Magic + version
    0x00, 0x61, 0x73, 0x6D,
    0x01, 0x00, 0x00, 0x00,

    // Type section
    0x01, 0x07,
    0x01,
    0x60,
    0x02,
    0x7F,
    0x7F,
    0x01,
    0x7F,

    // Function section
    0x03, 0x02,
    0x01,
    0x00,

    // Export section
    0x07, 0x07,
    0x01,
    0x03,
    0x61, 0x64, 0x64,
    0x00,
    0x00,

    // Code section
    0x0A, 0x09,
    0x01,
    0x07,
    0x00,
    0x20, 0x00,
    0x20, 0x01,
    0x6A,
    0x0B
};


// ------------------------------------------------------------
// Test helpers
// ------------------------------------------------------------

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


bool RequireError(
    const nexus::ErrorCode actual,
    const nexus::ErrorCode expected,
    const char* message
)
{
    if(actual == expected)
    {
        return true;
    }

    std::cerr
        << "FAILED: "
        << message
        << '\n'
        << "Expected error code: "
        << static_cast<int>(expected)
        << '\n'
        << "Actual error code: "
        << static_cast<int>(actual)
        << '\n';

    return false;
}

} // namespace


int main()
{
    // ========================================================
    // TEST 1
    //
    // Basic hierarchy:
    //
    // Runtime
    //   -> Realm
    //      -> Program
    //         -> Main ExecutionDomain
    //            -> Instance
    //
    // add(20, 22) == 42
    // ========================================================

    auto runtimeResult =
        nexus::Runtime::Create();

    if(!Require(
        static_cast<bool>(runtimeResult),
        "Runtime creation failed"
    ))
    {
        return 1;
    }

    auto runtime =
        std::move(runtimeResult).Value();


    auto realmResult =
        runtime.CreateRealm();

    if(!Require(
        static_cast<bool>(realmResult),
        "Realm creation failed"
    ))
    {
        return 2;
    }

    auto realm =
        std::move(realmResult).Value();


    auto programResult =
        realm.CreateProgram();

    if(!Require(
        static_cast<bool>(programResult),
        "Program creation failed"
    ))
    {
        return 3;
    }

    auto program =
        std::move(programResult).Value();


    auto moduleResult =
        runtime.Compile(
            AddModule,
            sizeof(AddModule)
        );

    if(!Require(
        static_cast<bool>(moduleResult),
        "Module compilation failed"
    ))
    {
        return 4;
    }

    auto module =
        std::move(moduleResult).Value();


    auto instanceResult =
        program
            .MainDomain()
            .Instantiate(module);

    if(!Require(
        static_cast<bool>(instanceResult),
        "Module instantiation failed"
    ))
    {
        return 5;
    }

    auto instance =
        std::move(instanceResult).Value();


    auto addResult =
        instance.Call<std::int32_t>(
            "add",
            std::int32_t{20},
            std::int32_t{22}
        );

    if(!Require(
        static_cast<bool>(addResult),
        "add() call failed"
    ))
    {
        return 6;
    }


    if(!Require(
        addResult.Value() == 42,
        "add(20, 22) must equal 42"
    ))
    {
        return 7;
    }


    // ========================================================
    // TEST 2
    //
    // Module reuse:
    //
    // A compiled Module is Engine-scoped, not Program-scoped.
    //
    // The exact same Module must be reusable by multiple
    // Programs / ExecutionDomains inside the same Runtime.
    // ========================================================

    auto realmTwoResult =
        runtime.CreateRealm();

    if(!Require(
        static_cast<bool>(realmTwoResult),
        "Second Realm creation failed"
    ))
    {
        return 8;
    }

    auto realmTwo =
        std::move(realmTwoResult).Value();


    auto programTwoResult =
        realmTwo.CreateProgram();

    if(!Require(
        static_cast<bool>(programTwoResult),
        "Second Program creation failed"
    ))
    {
        return 9;
    }

    auto programTwo =
        std::move(programTwoResult).Value();


    auto instanceTwoResult =
        programTwo
            .MainDomain()
            .Instantiate(module);

    if(!Require(
        static_cast<bool>(instanceTwoResult),
        "Module reuse in second Program failed"
    ))
    {
        return 10;
    }

    auto instanceTwo =
        std::move(instanceTwoResult).Value();


    auto secondCall =
        instanceTwo.Call<std::int32_t>(
            "add",
            std::int32_t{40},
            std::int32_t{2}
        );

    if(!Require(
        static_cast<bool>(secondCall),
        "Second Program call failed"
    ))
    {
        return 11;
    }


    if(!Require(
        secondCall.Value() == 42,
        "Second Program returned wrong value"
    ))
    {
        return 12;
    }


    // ========================================================
    // TEST 3
    //
    // Module lifetime:
    //
    // Destroying the Module handle after instantiation must
    // NOT destroy the already-created Wasm Instance.
    //
    // Instance lifetime belongs to the Store.
    // ========================================================

    std::optional<nexus::Instance>
        instanceAfterModuleDestruction;


    {
        auto temporaryModuleResult =
            runtime.Compile(
                AddModule,
                sizeof(AddModule)
            );

        if(!Require(
            static_cast<bool>(
                temporaryModuleResult
            ),
            "Temporary Module compilation failed"
        ))
        {
            return 13;
        }


        auto temporaryModule =
            std::move(
                temporaryModuleResult
            ).Value();


        auto temporaryInstanceResult =
            program
                .MainDomain()
                .Instantiate(
                    temporaryModule
                );

        if(!Require(
            static_cast<bool>(
                temporaryInstanceResult
            ),
            "Temporary Module instantiation failed"
        ))
        {
            return 14;
        }


        instanceAfterModuleDestruction.emplace(
            std::move(
                temporaryInstanceResult
            ).Value()
        );

    } // temporaryModule destroyed here


    auto callAfterModuleDestruction =
        instanceAfterModuleDestruction
            ->Call<std::int32_t>(
                "add",
                std::int32_t{20},
                std::int32_t{22}
            );


    if(!Require(
        callAfterModuleDestruction
        &&
        callAfterModuleDestruction.Value() == 42,
        "Instance became invalid after Module destruction"
    ))
    {
        return 15;
    }


    // ========================================================
    // TEST 4
    //
    // Realm handle lifetime:
    //
    // Destroying the Realm handle must NOT invalidate a
    // Program that was already created from it.
    //
    // Program keeps RealmState alive.
    // ========================================================

    std::optional<nexus::Program>
        survivingProgram;


    {
        auto temporaryRealmResult =
            runtime.CreateRealm();

        if(!Require(
            static_cast<bool>(
                temporaryRealmResult
            ),
            "Temporary Realm creation failed"
        ))
        {
            return 16;
        }


        auto temporaryRealm =
            std::move(
                temporaryRealmResult
            ).Value();


        auto survivingProgramResult =
            temporaryRealm.CreateProgram();

        if(!Require(
            static_cast<bool>(
                survivingProgramResult
            ),
            "Program creation in temporary Realm failed"
        ))
        {
            return 17;
        }


        survivingProgram.emplace(
            std::move(
                survivingProgramResult
            ).Value()
        );

    } // Realm handle destroyed here


    auto survivingInstanceResult =
        survivingProgram
            ->MainDomain()
            .Instantiate(module);


    if(!Require(
        static_cast<bool>(
            survivingInstanceResult
        ),
        "Program became invalid after Realm handle destruction"
    ))
    {
        return 18;
    }


    auto survivingInstance =
        std::move(
            survivingInstanceResult
        ).Value();


    auto survivingCall =
        survivingInstance.Call<std::int32_t>(
            "add",
            std::int32_t{21},
            std::int32_t{21}
        );


    if(!Require(
        survivingCall
        &&
        survivingCall.Value() == 42,
        "Call failed after Realm handle destruction"
    ))
    {
        return 19;
    }


    // ========================================================
    // TEST 5
    //
    // Program destruction:
    //
    // Program owns its Main ExecutionDomain.
    //
    // Instance must NOT keep ExecutionDomain / Store alive.
    //
    // Therefore an Instance retained after Program destruction
    // must fail safely with InvalidState.
    // ========================================================

    std::optional<nexus::Instance>
        orphanInstance;


    {
        auto temporaryRealmResult =
            runtime.CreateRealm();

        if(!Require(
            static_cast<bool>(
                temporaryRealmResult
            ),
            "Realm creation for Program destruction test failed"
        ))
        {
            return 20;
        }


        auto temporaryRealm =
            std::move(
                temporaryRealmResult
            ).Value();


        auto temporaryProgramResult =
            temporaryRealm.CreateProgram();

        if(!Require(
            static_cast<bool>(
                temporaryProgramResult
            ),
            "Program creation for destruction test failed"
        ))
        {
            return 21;
        }


        auto temporaryProgram =
            std::move(
                temporaryProgramResult
            ).Value();


        auto orphanInstanceResult =
            temporaryProgram
                .MainDomain()
                .Instantiate(module);

        if(!Require(
            static_cast<bool>(
                orphanInstanceResult
            ),
            "Instance creation for destruction test failed"
        ))
        {
            return 22;
        }


        orphanInstance.emplace(
            std::move(
                orphanInstanceResult
            ).Value()
        );


        auto callBeforeDestruction =
            orphanInstance
                ->Call<std::int32_t>(
                    "add",
                    std::int32_t{20},
                    std::int32_t{22}
                );


        if(!Require(
            callBeforeDestruction
            &&
            callBeforeDestruction.Value() == 42,
            "Instance failed before Program destruction"
        ))
        {
            return 23;
        }

    } // temporaryProgram destroyed here
      // Main ExecutionDomain destroyed
      // Linker destroyed
      // Store destroyed


    auto callAfterProgramDestruction =
        orphanInstance
            ->Call<std::int32_t>(
                "add",
                std::int32_t{20},
                std::int32_t{22}
            );


    if(!Require(
        !callAfterProgramDestruction,
        "Instance unexpectedly survived Program destruction"
    ))
    {
        return 24;
    }


    if(!RequireError(
        callAfterProgramDestruction
            .GetError()
            .Code(),

        nexus::ErrorCode::InvalidState,

        "Wrong error after Program destruction"
    ))
    {
        return 25;
    }


    // ========================================================
    // TEST 6
    //
    // Runtime handle destruction:
    //
    // Existing children keep EngineState alive.
    //
    // Therefore a Program + Module created before Runtime
    // destruction must remain usable.
    // ========================================================

    std::optional<nexus::Program>
        programAfterRuntimeDestruction;

    std::optional<nexus::Module>
        moduleAfterRuntimeDestruction;


    {
        auto temporaryRuntimeResult =
            nexus::Runtime::Create();

        if(!Require(
            static_cast<bool>(
                temporaryRuntimeResult
            ),
            "Temporary Runtime creation failed"
        ))
        {
            return 26;
        }


        auto temporaryRuntime =
            std::move(
                temporaryRuntimeResult
            ).Value();


        auto temporaryRealmResult =
            temporaryRuntime.CreateRealm();

        if(!Require(
            static_cast<bool>(
                temporaryRealmResult
            ),
            "Temporary Runtime Realm creation failed"
        ))
        {
            return 27;
        }


        auto temporaryRealm =
            std::move(
                temporaryRealmResult
            ).Value();


        auto temporaryProgramResult =
            temporaryRealm.CreateProgram();

        if(!Require(
            static_cast<bool>(
                temporaryProgramResult
            ),
            "Temporary Runtime Program creation failed"
        ))
        {
            return 28;
        }


        programAfterRuntimeDestruction.emplace(
            std::move(
                temporaryProgramResult
            ).Value()
        );


        auto temporaryModuleResult =
            temporaryRuntime.Compile(
                AddModule,
                sizeof(AddModule)
            );

        if(!Require(
            static_cast<bool>(
                temporaryModuleResult
            ),
            "Temporary Runtime Module compilation failed"
        ))
        {
            return 29;
        }


        moduleAfterRuntimeDestruction.emplace(
            std::move(
                temporaryModuleResult
            ).Value()
        );

    } // Runtime + Realm handles destroyed here


    auto runtimeDetachedInstanceResult =
        programAfterRuntimeDestruction
            ->MainDomain()
            .Instantiate(
                *moduleAfterRuntimeDestruction
            );


    if(!Require(
        static_cast<bool>(
            runtimeDetachedInstanceResult
        ),
        "Program/Module invalidated by Runtime handle destruction"
    ))
    {
        return 30;
    }


    auto runtimeDetachedInstance =
        std::move(
            runtimeDetachedInstanceResult
        ).Value();


    auto runtimeDetachedCall =
        runtimeDetachedInstance
            .Call<std::int32_t>(
                "add",
                std::int32_t{20},
                std::int32_t{22}
            );


    if(!Require(
        runtimeDetachedCall
        &&
        runtimeDetachedCall.Value() == 42,
        "Call failed after Runtime handle destruction"
    ))
    {
        return 31;
    }


    // ========================================================
    // TEST 7
    //
    // Runtime / Engine mismatch:
    //
    // A Module compiled by Runtime A must not be instantiated
    // inside an ExecutionDomain created by Runtime B.
    // ========================================================

    auto foreignRuntimeResult =
        nexus::Runtime::Create();

    if(!Require(
        static_cast<bool>(
            foreignRuntimeResult
        ),
        "Foreign Runtime creation failed"
    ))
    {
        return 32;
    }


    auto foreignRuntime =
        std::move(
            foreignRuntimeResult
        ).Value();


    auto foreignRealmResult =
        foreignRuntime.CreateRealm();

    if(!Require(
        static_cast<bool>(
            foreignRealmResult
        ),
        "Foreign Realm creation failed"
    ))
    {
        return 33;
    }


    auto foreignRealm =
        std::move(
            foreignRealmResult
        ).Value();


    auto foreignProgramResult =
        foreignRealm.CreateProgram();

    if(!Require(
        static_cast<bool>(
            foreignProgramResult
        ),
        "Foreign Program creation failed"
    ))
    {
        return 34;
    }


    auto foreignProgram =
        std::move(
            foreignProgramResult
        ).Value();


    auto mismatchedInstance =
        foreignProgram
            .MainDomain()
            .Instantiate(module);


    if(!Require(
        !mismatchedInstance,
        "Cross-Runtime Module unexpectedly instantiated"
    ))
    {
        return 35;
    }


    if(!RequireError(
        mismatchedInstance
            .GetError()
            .Code(),

        nexus::ErrorCode::RuntimeMismatch,

        "Cross-Runtime Module returned wrong error"
    ))
    {
        return 36;
    }


    // ========================================================
    // TEST 8
    //
    // Final sanity:
    //
    // Original hierarchy must still be alive and usable after
    // all previous lifecycle tests.
    // ========================================================

    auto finalCall =
        instance.Call<std::int32_t>(
            "add",
            std::int32_t{10},
            std::int32_t{32}
        );


    if(!Require(
        finalCall
        &&
        finalCall.Value() == 42,
        "Original Instance became corrupted"
    ))
    {
        return 37;
    }


    // --------------------------------------------------------
    // Success
    // --------------------------------------------------------

    std::cout
        << "NexusWasm Realm/Program architecture OK\n"
        << '\n'
        << "Verified:\n"
        << "  Runtime -> Realm -> Program -> ExecutionDomain\n"
        << "  add(20, 22) == 42\n"
        << "  Module reuse across Programs\n"
        << "  Module destruction safety\n"
        << "  Realm handle destruction safety\n"
        << "  Program destruction invalidates Instance safely\n"
        << "  Runtime handle destruction safety\n"
        << "  Cross-Runtime Module rejection\n"
        << '\n';

    return 0;
}
