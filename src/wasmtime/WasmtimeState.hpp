#pragma once

#ifndef NEXUSWASM_WASMTIME_STATE_HEADER
#define NEXUSWASM_WASMTIME_STATE_HEADER

#include <memory>

#include <wasmtime.h>

namespace nexus::detail
{
    struct EngineDeleter final
    {
        void operator()(wasm_engine_t* engine) const noexcept
        {
            if (engine != nullptr)
            {
                wasm_engine_delete(engine);
            }
        }
    };

    struct ModuleDeleter final
    {
        void operator()(wasmtime_module_t* module) const noexcept
        {
            if (module != nullptr)
            {
                wasmtime_module_delete(module);
            }
        }
    };

    struct StoreDeleter final
    {
        void operator()(wasmtime_store_t* store) const noexcept
        {
            if (store != nullptr)
            {
                wasmtime_store_delete(store);
            }
        }
    };

    using EnginePtr = std::unique_ptr<wasm_engine_t, EngineDeleter>;
    using ModulePtr = std::unique_ptr<wasmtime_module_t, ModuleDeleter>;
    using StorePtr = std::unique_ptr<wasmtime_store_t, StoreDeleter>;

    struct EngineState final
    {
        EnginePtr engine;

        explicit EngineState(wasm_engine_t* rawEngine):
            engine{ rawEngine }
        {}
    };

    struct ModuleState final
    {
        std::shared_ptr<EngineState> engine;

        ModulePtr module;

        ModuleState(
            std::shared_ptr<EngineState> engineState,
            wasmtime_module_t*           rawModule
        ):
            engine{ std::move(engineState) },
            module{ rawModule }
        {}
    };

    struct InstanceState final
    {
        std::shared_ptr<EngineState> engine;

        StorePtr            store;
        wasmtime_instance_t instance{};

        InstanceState(
            std::shared_ptr<EngineState> engineState,
            wasmtime_store_t*            rawStore,
            const wasmtime_instance_t    rawInstance
        ):
            engine{ std::move(engineState) },
            store{ rawStore },
            instance{ rawInstance }
        {}
    };
}

#endif
