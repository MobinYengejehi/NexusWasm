#include <nexuswasm/AsyncCall.hpp>
#include <nexuswasm/AsyncHostFunction.hpp>
#include <nexuswasm/AsyncInstantiation.hpp>
#include <nexuswasm/Error.hpp>
#include <nexuswasm/ExecutionDomain.hpp>
#include <nexuswasm/Instance.hpp>
#include <nexuswasm/Module.hpp>
#include <nexuswasm/Program.hpp>
#include <nexuswasm/Realm.hpp>
#include <nexuswasm/Runtime.hpp>

#include "TestWat.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

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
            << "Expected error code: "
            << static_cast<int>(
                expected
            )
            << '\n'
            << "Actual error code: "
            << static_cast<int>(
                error.Code()
            )
            << '\n'
            << "Message: "
            << error.Message()
            << '\n';


        return false;
    }


    nexus::Result<nexus::Module>
    CompileFixture(
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


    nexus::Result<nexus::Program>
    CreateProgram(
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


    template<typename AsyncOperation>
    bool PollUntilReady(
        AsyncOperation& operation,

        std::size_t* pendingCount =
            nullptr,

        const std::size_t maxPolls =
            64
    )
    {
        std::size_t localPending =
            0;


        for(
            std::size_t i = 0;
            i < maxPolls;
            ++i
        )
        {
            auto pollResult =
                operation.Poll();


            if(!pollResult)
            {
                std::cerr
                    << "FAILED: async Poll(): "
                    << pollResult
                        .GetError()
                        .Message()
                    << '\n';


                return false;
            }


            if(
                pollResult.Value()
                ==
                nexus::AsyncPollStatus::Ready
            )
            {
                if(pendingCount != nullptr)
                {
                    *pendingCount =
                        localPending;
                }


                return true;
            }


            ++localPending;
        }


        std::cerr
            << "FAILED: async operation did not "
               "complete within poll limit\n";


        return false;
    }


    struct DelayedAddState final
    {
        std::size_t polls =
            0;


        std::int32_t value =
            0;
    };


    int RunTests()
    {
        // ====================================================
        // Runtime / modules
        // ====================================================

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


        auto addModuleResult =
            CompileFixture(
                runtime,
                "add.wat"
            );


        auto trapModuleResult =
            CompileFixture(
                runtime,
                "trap.wat"
            );


        auto delayedModuleResult =
            CompileFixture(
                runtime,
                "delayed_add.wat"
            );


        if(
            !RequireResult(
                addModuleResult,
                "add.wat compilation failed"
            )
            ||
            !RequireResult(
                trapModuleResult,
                "trap.wat compilation failed"
            )
            ||
            !RequireResult(
                delayedModuleResult,
                "delayed_add.wat compilation failed"
            )
        )
        {
            return 2;
        }


        auto addModule =
            std::move(
                addModuleResult
            ).Value();


        auto trapModule =
            std::move(
                trapModuleResult
            ).Value();


        auto delayedModule =
            std::move(
                delayedModuleResult
            ).Value();


        // ====================================================
        // A. Async exported add()
        // ====================================================

        auto basicProgramResult =
            CreateProgram(
                runtime
            );


        if(!RequireResult(
            basicProgramResult,
            "Basic Program creation failed"
        ))
        {
            return 3;
        }


        auto basicProgram =
            std::move(
                basicProgramResult
            ).Value();


        auto basicInstanceResult =
            basicProgram
                .MainDomain()
                .Instantiate(
                    addModule
                );


        if(!RequireResult(
            basicInstanceResult,
            "Basic Instance creation failed"
        ))
        {
            return 4;
        }


        auto basicInstance =
            std::move(
                basicInstanceResult
            ).Value();


        auto asyncAddResult =
            basicInstance
                .CallAsync<std::int32_t>(
                    "add",

                    std::int32_t{
                        20
                    },

                    std::int32_t{
                        22
                    }
                );


        if(!RequireResult(
            asyncAddResult,
            "CallAsync(add) failed"
        ))
        {
            return 5;
        }


        auto asyncAdd =
            std::move(
                asyncAddResult
            ).Value();


        if(!PollUntilReady(
            asyncAdd
        ))
        {
            return 6;
        }


        auto addResult =
            asyncAdd.TakeResult();


        if(
            !RequireResult(
                addResult,
                "Async add result failed"
            )
            ||
            !Require(
                addResult.Value() == 42,
                "Async add did not return 42"
            )
        )
        {
            return 7;
        }


        // K. Pure async call did NOT permanently make
        // this Store async-only.

        auto syncAfterAsync =
            basicInstance
                .Call<std::int32_t>(
                    "add",

                    std::int32_t{
                        10
                    },

                    std::int32_t{
                        32
                    }
                );


        if(
            !RequireResult(
                syncAfterAsync,
                "Sync call failed after completed async call"
            )
            ||
            !Require(
                syncAfterAsync.Value() == 42,
                "Sync call after async returned wrong value"
            )
        )
        {
            return 8;
        }


        // ====================================================
        // C. Trap propagation
        // ====================================================

        auto trapProgramResult =
            CreateProgram(
                runtime
            );


        if(!RequireResult(
            trapProgramResult,
            "Trap Program creation failed"
        ))
        {
            return 9;
        }


        auto trapProgram =
            std::move(
                trapProgramResult
            ).Value();


        auto trapInstanceResult =
            trapProgram
                .MainDomain()
                .Instantiate(
                    trapModule
                );


        if(!RequireResult(
            trapInstanceResult,
            "Trap Instance creation failed"
        ))
        {
            return 10;
        }


        auto trapInstance =
            std::move(
                trapInstanceResult
            ).Value();


        auto asyncTrapResult =
            trapInstance
                .CallAsync<std::int32_t>(
                    "boom"
                );


        if(!RequireResult(
            asyncTrapResult,
            "CallAsync(boom) creation failed"
        ))
        {
            return 11;
        }


        auto asyncTrap =
            std::move(
                asyncTrapResult
            ).Value();


        if(!PollUntilReady(
            asyncTrap
        ))
        {
            return 12;
        }


        auto trappedResult =
            asyncTrap.TakeResult();


        if(
            !Require(
                !trappedResult,
                "Async trap unexpectedly succeeded"
            )
            ||
            !RequireError(
                trappedResult.GetError(),

                nexus::ErrorCode::Trap,

                "Wrong async trap error"
            )
        )
        {
            return 13;
        }


        // ====================================================
        // D. Invalid export / signature
        // ====================================================

        auto missingExport =
            basicInstance
                .CallAsync<std::int32_t>(
                    "does_not_exist"
                );


        if(
            !Require(
                !missingExport,
                "Missing async export unexpectedly succeeded"
            )
            ||
            !RequireError(
                missingExport.GetError(),

                nexus::ErrorCode::ExportNotFound,

                "Wrong missing-export error"
            )
        )
        {
            return 14;
        }


        auto wrongSignature =
            basicInstance
                .CallAsync<std::int64_t>(
                    "add",

                    std::int32_t{
                        20
                    },

                    std::int32_t{
                        22
                    }
                );


        if(
            !Require(
                !wrongSignature,
                "Wrong async signature unexpectedly succeeded"
            )
            ||
            !RequireError(
                wrongSignature.GetError(),

                nexus::ErrorCode::SignatureMismatch,

                "Wrong async signature error"
            )
        )
        {
            return 15;
        }


        // ====================================================
        // E/F/G/H/J/K
        //
        // Future alive:
        // - second async rejected
        // - sync rejected
        // - ModuleGraph rejected
        // - another Store remains independent
        //
        // Future destroyed while pending:
        // - Store becomes usable again
        // - ModuleGraph works
        // ====================================================

        auto guardProgramResult =
            CreateProgram(
                runtime
            );


        if(!RequireResult(
            guardProgramResult,
            "Guard Program creation failed"
        ))
        {
            return 16;
        }


        auto guardProgram =
            std::move(
                guardProgramResult
            ).Value();


        auto guardInstanceResult =
            guardProgram
                .MainDomain()
                .Instantiate(
                    addModule
                );


        if(!RequireResult(
            guardInstanceResult,
            "Guard Instance creation failed"
        ))
        {
            return 17;
        }


        auto guardInstance =
            std::move(
                guardInstanceResult
            ).Value();


        const auto addGraphModule =
            guardProgram.AddModule(
                "math",
                addModule
            );


        if(!RequireResult(
            addGraphModule,
            "Adding ModuleGraph module failed"
        ))
        {
            return 18;
        }


        {
            auto activeFutureResult =
                guardInstance
                    .CallAsync<std::int32_t>(
                        "add",

                        std::int32_t{
                            20
                        },

                        std::int32_t{
                            22
                        }
                    );


            if(!RequireResult(
                activeFutureResult,
                "Active future creation failed"
            ))
            {
                return 19;
            }


            auto activeFuture =
                std::move(
                    activeFutureResult
                ).Value();


            // E

            auto secondAsync =
                guardInstance
                    .CallAsync<std::int32_t>(
                        "add",

                        std::int32_t{
                            1
                        },

                        std::int32_t{
                            1
                        }
                    );


            if(
                !Require(
                    !secondAsync,
                    "Second async call unexpectedly succeeded"
                )
                ||
                !RequireError(
                    secondAsync.GetError(),

                    nexus::ErrorCode::StoreBusy,

                    "Wrong second-async Store guard error"
                )
            )
            {
                return 20;
            }


            // F

            auto syncWhileAsync =
                guardInstance
                    .Call<std::int32_t>(
                        "add",

                        std::int32_t{
                            1
                        },

                        std::int32_t{
                            1
                        }
                    );


            if(
                !Require(
                    !syncWhileAsync,
                    "Sync call unexpectedly succeeded "
                    "while async future was active"
                )
                ||
                !RequireError(
                    syncWhileAsync.GetError(),

                    nexus::ErrorCode::StoreBusy,

                    "Wrong sync Store guard error"
                )
            )
            {
                return 21;
            }


            // J

            auto graphWhileAsync =
                guardProgram.Instantiate();


            if(
                !Require(
                    !graphWhileAsync,
                    "ModuleGraph unexpectedly instantiated "
                    "while async future was active"
                )
                ||
                !RequireError(
                    graphWhileAsync.GetError(),

                    nexus::ErrorCode::StoreBusy,

                    "Wrong ModuleGraph Store guard error"
                )
            )
            {
                return 22;
            }


            // G — second Store is independent.

            auto independentProgramResult =
                CreateProgram(
                    runtime
                );


            if(!RequireResult(
                independentProgramResult,
                "Independent Program creation failed"
            ))
            {
                return 23;
            }


            auto independentProgram =
                std::move(
                    independentProgramResult
                ).Value();


            auto independentInstanceResult =
                independentProgram
                    .MainDomain()
                    .Instantiate(
                        addModule
                    );


            if(!RequireResult(
                independentInstanceResult,
                "Independent Store instantiation failed"
            ))
            {
                return 24;
            }


            auto independentInstance =
                std::move(
                    independentInstanceResult
                ).Value();


            auto independentCall =
                independentInstance
                    .Call<std::int32_t>(
                        "add",

                        std::int32_t{
                            20
                        },

                        std::int32_t{
                            22
                        }
                    );


            if(
                !RequireResult(
                    independentCall,
                    "Second Store call failed"
                )
                ||
                !Require(
                    independentCall.Value() == 42,
                    "Second Store returned wrong value"
                )
            )
            {
                return 25;
            }


            // H:
            //
            // no Poll().
            // activeFuture destructor deletes native
            // wasmtime_call_future_t while pending.

        }


        // K — guard released after future destruction.

        auto graphAfterCleanup =
            guardProgram.Instantiate();


        if(!RequireResult(
            graphAfterCleanup,
            "ModuleGraph failed after future cleanup"
        ))
        {
            return 26;
        }


        auto graphInstanceResult =
            guardProgram.GetInstance(
                "math"
            );


        if(!RequireResult(
            graphInstanceResult,
            "Graph instance lookup failed"
        ))
        {
            return 27;
        }


        auto graphInstance =
            std::move(
                graphInstanceResult
            ).Value();


        auto graphCall =
            graphInstance
                .Call<std::int32_t>(
                    "add",

                    std::int32_t{
                        20
                    },

                    std::int32_t{
                        22
                    }
                );


        if(
            !RequireResult(
                graphCall,
                "Call failed after future cleanup"
            )
            ||
            !Require(
                graphCall.Value() == 42,
                "Call after future cleanup returned wrong value"
            )
        )
        {
            return 28;
        }


        // ====================================================
        // I. Native async host import requiring several polls
        // ====================================================

        auto hostProgramResult =
            CreateProgram(
                runtime
            );


        if(!RequireResult(
            hostProgramResult,
            "Async-host Program creation failed"
        ))
        {
            return 29;
        }


        auto hostProgram =
            std::move(
                hostProgramResult
            ).Value();


        nexus::AsyncHostFunctionSignature
            delayedSignature;


        delayedSignature.parameters =
        {
            nexus::HostValueKind::I32,
            nexus::HostValueKind::I32
        };


        delayedSignature.results =
        {
            nexus::HostValueKind::I32
        };


        const auto defineResult =
            hostProgram
                .MainDomain()
                .DefineAsyncFunction(
                    "host",
                    "delayed_add",

                    delayedSignature,

                    [](
                        const std::vector<
                            nexus::HostValue
                        >& arguments
                    )
                    -> nexus::Result<
                        nexus::AsyncHostOperation
                    >
                    {
                        if(arguments.size() != 2)
                        {
                            return nexus::Error{
                                nexus::ErrorCode::InvalidState,

                                "delayed_add received "
                                "wrong argument count."
                            };
                        }


                        const auto* lhs =
                            std::get_if<
                                std::int32_t
                            >(
                                &arguments[0]
                            );


                        const auto* rhs =
                            std::get_if<
                                std::int32_t
                            >(
                                &arguments[1]
                            );


                        if(
                            lhs == nullptr
                            ||
                            rhs == nullptr
                        )
                        {
                            return nexus::Error{
                                nexus::ErrorCode::InvalidState,

                                "delayed_add received "
                                "wrong argument types."
                            };
                        }


                        auto state =
                            std::make_shared<
                                DelayedAddState
                            >();


                        state->value =
                            *lhs
                            +
                            *rhs;


                        return nexus::AsyncHostOperation{
                            [state]()
                            -> nexus::Result<
                                nexus::AsyncPollStatus
                            >
                            {
                                ++state->polls;


                                if(state->polls < 3)
                                {
                                    return nexus::
                                        AsyncPollStatus::
                                            Pending;
                                }


                                return nexus::
                                    AsyncPollStatus::
                                        Ready;
                            },


                            [state]()
                            -> nexus::Result<
                                std::vector<
                                    nexus::HostValue
                                >
                            >
                            {
                                return std::vector<
                                    nexus::HostValue
                                >{
                                    nexus::HostValue{
                                        state->value
                                    }
                                };
                            }
                        };
                    }
                );


        if(!RequireResult(
            defineResult,
            "DefineAsyncFunction failed"
        ))
        {
            return 30;
        }


        // Sync instantiation must be proactively rejected.

        auto incorrectSyncInstantiation =
            hostProgram
                .MainDomain()
                .Instantiate(
                    delayedModule
                );


        if(
            !Require(
                !incorrectSyncInstantiation,
                "Async-import module unexpectedly "
                "instantiated synchronously"
            )
            ||
            !RequireError(
                incorrectSyncInstantiation.GetError(),

                nexus::ErrorCode::StoreRequiresAsync,

                "Wrong sync-instantiation rejection"
            )
        )
        {
            return 31;
        }


        auto hostInstantiationResult =
            hostProgram
                .MainDomain()
                .InstantiateAsync(
                    delayedModule
                );


        if(!RequireResult(
            hostInstantiationResult,
            "InstantiateAsync failed"
        ))
        {
            return 32;
        }


        auto hostInstantiation =
            std::move(
                hostInstantiationResult
            ).Value();


        if(!PollUntilReady(
            hostInstantiation
        ))
        {
            return 33;
        }


        auto hostInstanceResult =
            hostInstantiation
                .TakeInstance();


        if(!RequireResult(
            hostInstanceResult,
            "TakeInstance failed"
        ))
        {
            return 34;
        }


        auto hostInstance =
            std::move(
                hostInstanceResult
            ).Value();


        // Store is now sticky async-required.

        auto syncOnAsyncStore =
            hostInstance
                .Call<std::int32_t>(
                    "run"
                );


        if(
            !Require(
                !syncOnAsyncStore,
                "Sync call unexpectedly succeeded "
                "on async-required Store"
            )
            ||
            !RequireError(
                syncOnAsyncStore.GetError(),

                nexus::ErrorCode::StoreRequiresAsync,

                "Wrong async-required Store error"
            )
        )
        {
            return 35;
        }


        auto delayedCallResult =
            hostInstance
                .CallAsync<std::int32_t>(
                    "run"
                );


        if(!RequireResult(
            delayedCallResult,
            "CallAsync(run) failed"
        ))
        {
            return 36;
        }


        auto delayedCall =
            std::move(
                delayedCallResult
            ).Value();


        std::size_t pendingCount =
            0;


        if(!PollUntilReady(
            delayedCall,
            &pendingCount
        ))
        {
            return 37;
        }


        if(!Require(
            pendingCount >= 2,
            "Async host continuation did not "
            "produce the expected Pending polls"
        ))
        {
            return 38;
        }


        auto delayedResult =
            delayedCall.TakeResult();


        if(
            !RequireResult(
                delayedResult,
                "Async host final result failed"
            )
            ||
            !Require(
                delayedResult.Value() == 42,
                "Async host operation did not return 42"
            )
        )
        {
            return 39;
        }


        // A second async call proves the Store returned
        // from Active → Idle while remaining async-required.

        auto secondDelayedResult =
            hostInstance
                .CallAsync<std::int32_t>(
                    "run"
                );


        if(!RequireResult(
            secondDelayedResult,
            "Second async-host call creation failed"
        ))
        {
            return 40;
        }


        auto secondDelayed =
            std::move(
                secondDelayedResult
            ).Value();


        if(!PollUntilReady(
            secondDelayed
        ))
        {
            return 41;
        }


        auto secondDelayedValue =
            secondDelayed.TakeResult();


        if(
            !RequireResult(
                secondDelayedValue,
                "Second async-host result failed"
            )
            ||
            !Require(
                secondDelayedValue.Value() == 42,
                "Second async-host call returned wrong value"
            )
        )
        {
            return 42;
        }


        std::cout
            << "NexusWasm native async OK\n"
            << '\n'
            << "Verified:\n"
            << "  async exported function call\n"
            << "  native future polling\n"
            << "  trap propagation\n"
            << "  export/signature validation\n"
            << "  one active future per Store\n"
            << "  sync rejection while future active\n"
            << "  independent Stores\n"
            << "  pending future destruction\n"
            << "  async host continuation\n"
            << "  ModuleGraph Store guard\n"
            << "  Store recovery after future cleanup\n"
            << "  sticky async-required Store mode\n"
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
    catch(
        const std::exception&
            exception
    )
    {
        std::cerr
            << "UNEXPECTED TEST FAILURE: "
            << exception.what()
            << '\n';


        return 100;
    }
}
