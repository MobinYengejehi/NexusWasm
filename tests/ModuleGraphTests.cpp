#include <nexuswasm/Error.hpp>
#include <nexuswasm/Instance.hpp>
#include <nexuswasm/Module.hpp>
#include <nexuswasm/Program.hpp>
#include <nexuswasm/Realm.hpp>
#include <nexuswasm/Runtime.hpp>

#include "TestWat.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string_view>
#include <utility>

namespace
{

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


template<typename T>
bool RequireResult(
    const nexus::Result<T>& result,
    const char* message
)
{
    if(result)
    {
        return true;
    }


    std::cerr
        << "FAILED: "
        << message
        << '\n'
        << "NexusWasm error: "
        << result.GetError().Message()
        << '\n';


    return false;
}


bool RequireError(
    const nexus::Error& error,
    const nexus::ErrorCode expected,
    const char* message
)
{
    if(error.Code() == expected)
    {
        return true;
    }


    std::cerr
        << "FAILED: "
        << message
        << '\n'
        << "Actual error message: "
        << error.Message()
        << '\n';


    return false;
}


nexus::Result<nexus::Module> CompileFixture(
    nexus::Runtime& runtime,
    const std::string_view fileName
)
{
    const auto wasm =
        nexus::test::WatFileToWasm(
            fileName
        );


    return runtime.Compile(
        wasm
    );
}


nexus::Result<nexus::Program> CreateProgram(
    nexus::Runtime& runtime
)
{
    auto realmResult =
        runtime.CreateRealm();


    if(!realmResult)
    {
        return realmResult.GetError();
    }


    auto realm =
        std::move(
            realmResult
        ).Value();


    return realm.CreateProgram();
}


int RunTests()
{
    // ========================================================
    // Runtime
    // ========================================================

    auto runtimeResult =
        nexus::Runtime::Create();


    if(!RequireResult(
        runtimeResult,
        "Runtime creation failed"
    ))
    {
        return 1;
    }


    auto runtime =
        std::move(
            runtimeResult
        ).Value();


    // ========================================================
    // Compile common modules
    // ========================================================

    auto mathResult =
        CompileFixture(
            runtime,
            "math.wat"
        );


    auto gameResult =
        CompileFixture(
            runtime,
            "game.wat"
        );


    if(
        !RequireResult(
            mathResult,
            "math.wat compilation failed"
        )
        ||
        !RequireResult(
            gameResult,
            "game.wat compilation failed"
        )
    )
    {
        return 2;
    }


    auto math =
        std::move(
            mathResult
        ).Value();


    auto game =
        std::move(
            gameResult
        ).Value();


    // ========================================================
    // TEST 1 — A -> B
    //
    // game -> math
    //
    // Registration is intentionally reverse-order.
    // ========================================================

    auto programResult =
        CreateProgram(
            runtime
        );


    if(!RequireResult(
        programResult,
        "Program creation failed"
    ))
    {
        return 3;
    }


    auto program =
        std::move(
            programResult
        ).Value();


    const auto addGame =
        program.AddModule(
            "game",
            game
        );


    if(!RequireResult(
        addGame,
        "Adding game module failed"
    ))
    {
        return 4;
    }


    const auto addMath =
        program.AddModule(
            "math",
            math
        );


    if(!RequireResult(
        addMath,
        "Adding math module failed"
    ))
    {
        return 5;
    }


    auto instantiateResult =
        program.Instantiate();


    if(!RequireResult(
        instantiateResult,
        "A -> B graph instantiation failed"
    ))
    {
        return 6;
    }


    auto gameInstanceResult =
        program.GetInstance(
            "game"
        );


    if(!RequireResult(
        gameInstanceResult,
        "game instance lookup failed"
    ))
    {
        return 7;
    }


    auto gameInstance =
        std::move(
            gameInstanceResult
        ).Value();


    auto runResult =
        gameInstance.Call<std::int32_t>(
            "run"
        );


    if(
        !RequireResult(
            runResult,
            "game.run() failed"
        )
        ||
        !Require(
            runResult.Value() == 42,
            "game.run() did not return 42"
        )
    )
    {
        return 8;
    }


    // ========================================================
    // TEST 2 — A -> B -> C
    //
    // game_chain -> physics -> math
    // ========================================================

    auto physicsResult =
        CompileFixture(
            runtime,
            "physics.wat"
        );


    auto chainGameResult =
        CompileFixture(
            runtime,
            "game_chain.wat"
        );


    auto chainProgramResult =
        CreateProgram(
            runtime
        );


    if(
        !RequireResult(
            physicsResult,
            "physics.wat compilation failed"
        )
        ||
        !RequireResult(
            chainGameResult,
            "game_chain.wat compilation failed"
        )
        ||
        !RequireResult(
            chainProgramResult,
            "Chain Program creation failed"
        )
    )
    {
        return 9;
    }


    auto physics =
        std::move(
            physicsResult
        ).Value();


    auto chainGame =
        std::move(
            chainGameResult
        ).Value();


    auto chainProgram =
        std::move(
            chainProgramResult
        ).Value();


    const auto addChainGame =
        chainProgram.AddModule(
            "game",
            chainGame
        );


    if(!RequireResult(
        addChainGame,
        "Adding chain game failed"
    ))
    {
        return 10;
    }


    const auto addPhysics =
        chainProgram.AddModule(
            "physics",
            physics
        );


    if(!RequireResult(
        addPhysics,
        "Adding physics failed"
    ))
    {
        return 11;
    }


    const auto addChainMath =
        chainProgram.AddModule(
            "math",
            math
        );


    if(!RequireResult(
        addChainMath,
        "Adding chain math failed"
    ))
    {
        return 12;
    }


    auto chainInstantiate =
        chainProgram.Instantiate();


    if(!RequireResult(
        chainInstantiate,
        "A -> B -> C graph failed"
    ))
    {
        return 13;
    }


    auto chainInstanceResult =
        chainProgram.GetInstance(
            "game"
        );


    if(!RequireResult(
        chainInstanceResult,
        "Chain game lookup failed"
    ))
    {
        return 14;
    }


    auto chainInstance =
        std::move(
            chainInstanceResult
        ).Value();


    auto chainCall =
        chainInstance.Call<std::int32_t>(
            "run"
        );


    if(
        !RequireResult(
            chainCall,
            "Chain call failed"
        )
        ||
        !Require(
            chainCall.Value() == 42,
            "A -> B -> C did not return 42"
        )
    )
    {
        return 15;
    }


    // ========================================================
    // TEST 3 — Independent branches
    //
    //          root
    //         /    \
    //      left    right
    // ========================================================

    auto leftResult =
        CompileFixture(
            runtime,
            "branch_left.wat"
        );


    auto rightResult =
        CompileFixture(
            runtime,
            "branch_right.wat"
        );


    auto rootResult =
        CompileFixture(
            runtime,
            "branch_root.wat"
        );


    auto branchProgramResult =
        CreateProgram(
            runtime
        );


    if(
        !RequireResult(
            leftResult,
            "branch_left.wat compilation failed"
        )
        ||
        !RequireResult(
            rightResult,
            "branch_right.wat compilation failed"
        )
        ||
        !RequireResult(
            rootResult,
            "branch_root.wat compilation failed"
        )
        ||
        !RequireResult(
            branchProgramResult,
            "Branch Program creation failed"
        )
    )
    {
        return 16;
    }


    auto left =
        std::move(leftResult).Value();

    auto right =
        std::move(rightResult).Value();

    auto root =
        std::move(rootResult).Value();

    auto branchProgram =
        std::move(
            branchProgramResult
        ).Value();


    // Root first deliberately.

    const auto addRoot =
        branchProgram.AddModule(
            "root",
            root
        );


    if(!RequireResult(
        addRoot,
        "Adding root failed"
    ))
    {
        return 17;
    }


    const auto addRight =
        branchProgram.AddModule(
            "right",
            right
        );


    if(!RequireResult(
        addRight,
        "Adding right branch failed"
    ))
    {
        return 18;
    }


    const auto addLeft =
        branchProgram.AddModule(
            "left",
            left
        );


    if(!RequireResult(
        addLeft,
        "Adding left branch failed"
    ))
    {
        return 19;
    }


    auto branchInstantiate =
        branchProgram.Instantiate();


    if(!RequireResult(
        branchInstantiate,
        "Independent branch graph failed"
    ))
    {
        return 20;
    }


    auto rootInstanceResult =
        branchProgram.GetInstance(
            "root"
        );


    if(!RequireResult(
        rootInstanceResult,
        "Root instance lookup failed"
    ))
    {
        return 21;
    }


    auto rootInstance =
        std::move(
            rootInstanceResult
        ).Value();


    auto branchCall =
        rootInstance.Call<std::int32_t>(
            "run"
        );


    if(
        !RequireResult(
            branchCall,
            "Branch root call failed"
        )
        ||
        !Require(
            branchCall.Value() == 42,
            "Independent branches did not return 42"
        )
    )
    {
        return 22;
    }


    // ========================================================
    // TEST 4 — Duplicate namespace
    // ========================================================

    auto duplicateProgramResult =
        CreateProgram(
            runtime
        );


    if(!RequireResult(
        duplicateProgramResult,
        "Duplicate test Program creation failed"
    ))
    {
        return 23;
    }


    auto duplicateProgram =
        std::move(
            duplicateProgramResult
        ).Value();


    const auto firstRegistration =
        duplicateProgram.AddModule(
            "math",
            math
        );


    if(!RequireResult(
        firstRegistration,
        "First namespace registration failed"
    ))
    {
        return 24;
    }


    const auto duplicateRegistration =
        duplicateProgram.AddModule(
            "math",
            math
        );


    if(
        !Require(
            !duplicateRegistration,
            "Duplicate namespace unexpectedly succeeded"
        )
        ||
        !RequireError(
            duplicateRegistration.GetError(),
            nexus::ErrorCode::DuplicateModuleNamespace,
            "Wrong duplicate namespace error"
        )
    )
    {
        return 25;
    }


    // ========================================================
    // TEST 5 — Missing import
    // ========================================================

    auto missingModuleResult =
        CompileFixture(
            runtime,
            "missing_import.wat"
        );


    auto missingProgramResult =
        CreateProgram(
            runtime
        );


    if(
        !RequireResult(
            missingModuleResult,
            "missing_import.wat compilation failed"
        )
        ||
        !RequireResult(
            missingProgramResult,
            "Missing-import Program creation failed"
        )
    )
    {
        return 26;
    }


    auto missingModule =
        std::move(
            missingModuleResult
        ).Value();


    auto missingProgram =
        std::move(
            missingProgramResult
        ).Value();


    const auto addMissing =
        missingProgram.AddModule(
            "main",
            missingModule
        );


    if(!RequireResult(
        addMissing,
        "Adding missing-import module failed"
    ))
    {
        return 27;
    }


    auto missingInstantiate =
        missingProgram.Instantiate();


    if(
        !Require(
            !missingInstantiate,
            "Missing dependency unexpectedly succeeded"
        )
        ||
        !RequireError(
            missingInstantiate.GetError(),
            nexus::ErrorCode::UnresolvedImport,
            "Wrong missing dependency error"
        )
    )
    {
        return 28;
    }


    // ========================================================
    // TEST 6 — Wrong function signature
    // ========================================================

    auto wrongMathResult =
        CompileFixture(
            runtime,
            "wrong_math.wat"
        );


    auto signatureProgramResult =
        CreateProgram(
            runtime
        );


    if(
        !RequireResult(
            wrongMathResult,
            "wrong_math.wat compilation failed"
        )
        ||
        !RequireResult(
            signatureProgramResult,
            "Signature Program creation failed"
        )
    )
    {
        return 29;
    }


    auto wrongMath =
        std::move(
            wrongMathResult
        ).Value();


    auto signatureProgram =
        std::move(
            signatureProgramResult
        ).Value();


    const auto addSignatureGame =
        signatureProgram.AddModule(
            "game",
            game
        );


    if(!RequireResult(
        addSignatureGame,
        "Adding signature-test game failed"
    ))
    {
        return 30;
    }


    const auto addWrongMath =
        signatureProgram.AddModule(
            "math",
            wrongMath
        );


    if(!RequireResult(
        addWrongMath,
        "Adding wrong math module failed"
    ))
    {
        return 31;
    }


    auto signatureInstantiate =
        signatureProgram.Instantiate();


    if(
        !Require(
            !signatureInstantiate,
            "Wrong signature unexpectedly succeeded"
        )
        ||
        !RequireError(
            signatureInstantiate.GetError(),
            nexus::ErrorCode::ImportSignatureMismatch,
            "Wrong signature mismatch error"
        )
    )
    {
        return 32;
    }


    // ========================================================
    // TEST 7 — Circular dependency
    // ========================================================

    auto cycleAResult =
        CompileFixture(
            runtime,
            "cycle_a.wat"
        );


    auto cycleBResult =
        CompileFixture(
            runtime,
            "cycle_b.wat"
        );


    auto cycleProgramResult =
        CreateProgram(
            runtime
        );


    if(
        !RequireResult(
            cycleAResult,
            "cycle_a.wat compilation failed"
        )
        ||
        !RequireResult(
            cycleBResult,
            "cycle_b.wat compilation failed"
        )
        ||
        !RequireResult(
            cycleProgramResult,
            "Cycle Program creation failed"
        )
    )
    {
        return 33;
    }


    auto cycleA =
        std::move(
            cycleAResult
        ).Value();


    auto cycleB =
        std::move(
            cycleBResult
        ).Value();


    auto cycleProgram =
        std::move(
            cycleProgramResult
        ).Value();


    const auto addCycleA =
        cycleProgram.AddModule(
            "A",
            cycleA
        );


    if(!RequireResult(
        addCycleA,
        "Adding cycle A failed"
    ))
    {
        return 34;
    }


    const auto addCycleB =
        cycleProgram.AddModule(
            "B",
            cycleB
        );


    if(!RequireResult(
        addCycleB,
        "Adding cycle B failed"
    ))
    {
        return 35;
    }


    auto cycleInstantiate =
        cycleProgram.Instantiate();


    if(
        !Require(
            !cycleInstantiate,
            "Circular dependency unexpectedly succeeded"
        )
        ||
        !RequireError(
            cycleInstantiate.GetError(),
            nexus::ErrorCode::CyclicModuleDependency,
            "Wrong circular dependency error"
        )
    )
    {
        return 36;
    }


    // ========================================================
    // TEST 8 — Same compiled Module under multiple namespaces
    // ========================================================

    auto multiProgramResult =
        CreateProgram(
            runtime
        );


    if(!RequireResult(
        multiProgramResult,
        "Multi-instance Program creation failed"
    ))
    {
        return 37;
    }


    auto multiProgram =
        std::move(
            multiProgramResult
        ).Value();


    const auto addMathA =
        multiProgram.AddModule(
            "mathA",
            math
        );


    if(!RequireResult(
        addMathA,
        "Adding mathA failed"
    ))
    {
        return 38;
    }


    const auto addMathB =
        multiProgram.AddModule(
            "mathB",
            math
        );


    if(!RequireResult(
        addMathB,
        "Adding mathB failed"
    ))
    {
        return 39;
    }


    auto multiInstantiate =
        multiProgram.Instantiate();


    if(!RequireResult(
        multiInstantiate,
        "Multiple instances of same Module failed"
    ))
    {
        return 40;
    }


    auto mathAResult =
        multiProgram.GetInstance(
            "mathA"
        );


    auto mathBResult =
        multiProgram.GetInstance(
            "mathB"
        );


    if(
        !RequireResult(
            mathAResult,
            "mathA lookup failed"
        )
        ||
        !RequireResult(
            mathBResult,
            "mathB lookup failed"
        )
    )
    {
        return 41;
    }


    auto mathA =
        std::move(
            mathAResult
        ).Value();


    auto mathB =
        std::move(
            mathBResult
        ).Value();


    auto mathACall =
        mathA.Call<std::int32_t>(
            "add",
            std::int32_t{20},
            std::int32_t{22}
        );


    auto mathBCall =
        mathB.Call<std::int32_t>(
            "add",
            std::int32_t{40},
            std::int32_t{2}
        );


    if(
        !RequireResult(
            mathACall,
            "mathA call failed"
        )
        ||
        !Require(
            mathACall.Value() == 42,
            "mathA returned wrong result"
        )
        ||
        !RequireResult(
            mathBCall,
            "mathB call failed"
        )
        ||
        !Require(
            mathBCall.Value() == 42,
            "mathB returned wrong result"
        )
    )
    {
        return 42;
    }


    // ========================================================
    // TEST 9 — Module handle lifetime
    //
    // ModuleGraph stores its own Module copy.
    // Original public Module handles may disappear before
    // Program::Instantiate().
    // ========================================================

    auto lifetimeProgramResult =
        CreateProgram(
            runtime
        );


    if(!RequireResult(
        lifetimeProgramResult,
        "Lifetime Program creation failed"
    ))
    {
        return 43;
    }


    auto lifetimeProgram =
        std::move(
            lifetimeProgramResult
        ).Value();


    {
        auto temporaryMathResult =
            CompileFixture(
                runtime,
                "math.wat"
            );


        auto temporaryGameResult =
            CompileFixture(
                runtime,
                "game.wat"
            );


        if(
            !RequireResult(
                temporaryMathResult,
                "Temporary math compilation failed"
            )
            ||
            !RequireResult(
                temporaryGameResult,
                "Temporary game compilation failed"
            )
        )
        {
            return 44;
        }


        auto temporaryMath =
            std::move(
                temporaryMathResult
            ).Value();


        auto temporaryGame =
            std::move(
                temporaryGameResult
            ).Value();


        const auto addTemporaryGame =
            lifetimeProgram.AddModule(
                "game",
                temporaryGame
            );


        if(!RequireResult(
            addTemporaryGame,
            "Adding temporary game failed"
        ))
        {
            return 45;
        }


        const auto addTemporaryMath =
            lifetimeProgram.AddModule(
                "math",
                temporaryMath
            );


        if(!RequireResult(
            addTemporaryMath,
            "Adding temporary math failed"
        ))
        {
            return 46;
        }

    } // original Module handles destroyed


    auto lifetimeInstantiate =
        lifetimeProgram.Instantiate();


    if(!RequireResult(
        lifetimeInstantiate,
        "Graph lost Module lifetime ownership"
    ))
    {
        return 47;
    }


    auto lifetimeGameResult =
        lifetimeProgram.GetInstance(
            "game"
        );


    if(!RequireResult(
        lifetimeGameResult,
        "Lifetime game lookup failed"
    ))
    {
        return 48;
    }


    auto lifetimeGame =
        std::move(
            lifetimeGameResult
        ).Value();


    auto lifetimeCall =
        lifetimeGame.Call<std::int32_t>(
            "run"
        );


    if(
        !RequireResult(
            lifetimeCall,
            "Lifetime game call failed"
        )
        ||
        !Require(
            lifetimeCall.Value() == 42,
            "Module lifetime test returned wrong result"
        )
    )
    {
        return 49;
    }


    // ========================================================
    // TEST 10 — Invalid WASM
    // ========================================================

    constexpr std::uint8_t invalidWasm[] =
    {
        0x01,
        0x02,
        0x03,
        0x04
    };


    auto invalidResult =
        runtime.Compile(
            invalidWasm,
            sizeof(invalidWasm)
        );


    if(
        !Require(
            !invalidResult,
            "Invalid WASM unexpectedly compiled"
        )
        ||
        !RequireError(
            invalidResult.GetError(),
            nexus::ErrorCode::CompilationFailed,
            "Wrong invalid WASM error"
        )
    )
    {
        return 50;
    }


    std::cout
        << "NexusWasm Module Graph OK\n"
        << '\n'
        << "Verified:\n"
        << "  A -> B\n"
        << "  A -> B -> C\n"
        << "  independent graph branches\n"
        << "  dependency ordering\n"
        << "  duplicate namespace rejection\n"
        << "  missing dependency detection\n"
        << "  function signature validation\n"
        << "  cycle detection\n"
        << "  multiple instances of one compiled Module\n"
        << "  Module handle lifetime\n"
        << "  invalid WASM rejection\n"
        << '\n';


    return 0;
}

}


int main()
{
    try
    {
        return RunTests();
    }
    catch(const std::exception& exception)
    {
        std::cerr
            << "UNEXPECTED TEST FAILURE: "
            << exception.what()
            << '\n';


        return 100;
    }
}
