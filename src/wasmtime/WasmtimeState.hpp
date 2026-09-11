#pragma once

#ifndef NEXUSWASM_WASMTIME_STATE_HEADER
#define NEXUSWASM_WASMTIME_STATE_HEADER

#include "module/ModuleMetadata.hpp"

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

    struct LinkerDeleter final
    {
        void operator()(wasmtime_linker_t* linker) const noexcept
        {
            if (linker != nullptr)
            {
                wasmtime_linker_delete(linker);
            }
        }
    };

    using EnginePtr = std::unique_ptr<wasm_engine_t, EngineDeleter>;
    using ModulePtr = std::unique_ptr<wasmtime_module_t, ModuleDeleter>;
    using StorePtr = std::unique_ptr<wasmtime_store_t, StoreDeleter>;
    using LinkerPtr = std::unique_ptr<wasmtime_linker_t, LinkerDeleter>;

    struct EngineState final
    {
        EnginePtr engine;

        explicit EngineState(wasm_engine_t* rawEngine):
            engine{ rawEngine }
        {}
    };

    struct RealmState final
    {
        std::shared_ptr<EngineState> engine;

        explicit RealmState(std::shared_ptr<EngineState> engineState):
            engine{ std::move(engineState) }
        {}
    };

    struct ModuleState final
    {
        std::shared_ptr<EngineState> engine;

        ModulePtr      module;
        ModuleMetadata metadata;

        ModuleState(
            std::shared_ptr<EngineState> engineState,
            wasmtime_module_t*           rawModule,
            ModuleMetadata               moduleMetadata
        ):
            engine{ std::move(engineState) },
            module{ rawModule },
            metadata{ std::move(moduleMetadata) }
        {}
    };

    struct ExecutionDomainState final
    {
        std::shared_ptr<EngineState> engine;

        StorePtr  store;
        LinkerPtr linker;

        ExecutionDomainState(
            std::shared_ptr<EngineState> engineState,
            wasmtime_store_t*            rawStore,
            wasmtime_linker_t*           rawLinker
        ):
            engine{ std::move(engineState) },
            store{ rawStore },
            linker{ rawLinker }
        {}
    };

    struct InstanceState final
    {
        std::weak_ptr<ExecutionDomainState> domain;

        wasmtime_instance_t instance{};

        InstanceState(
            const std::shared_ptr<ExecutionDomainState>& domainState,
            const wasmtime_instance_t                    rawInstance
        ):
            domain{ domainState },
            instance{ rawInstance }
        {}
    };
}

#endif
