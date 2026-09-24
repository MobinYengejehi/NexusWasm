#pragma once

#ifndef NEXUSWASM_WASMTIME_STATE_HEADER
#define NEXUSWASM_WASMTIME_STATE_HEADER

#include "execution/StoreExecutionState.hpp"
#include "module/ModuleMetadata.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

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

    struct CallFutureDeleter final
    {
        void operator()(wasmtime_call_future_t* future) const noexcept
        {
            if (future != nullptr)
            {
                wasmtime_call_future_delete(future);
            }
        }
    };

    using EnginePtr = std::unique_ptr<wasm_engine_t, EngineDeleter>;
    using ModulePtr = std::unique_ptr<wasmtime_module_t, ModuleDeleter>;
    using StorePtr = std::unique_ptr<wasmtime_store_t, StoreDeleter>;
    using LinkerPtr = std::unique_ptr<wasmtime_linker_t, LinkerDeleter>;
    using CallFuturePtr = std::unique_ptr<wasmtime_call_future_t, CallFutureDeleter>;

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

        StoreExecutionState executionState;

        StorePtr  store;
        LinkerPtr linker;

        std::unordered_set<std::string> asyncHostFunctions;

        ExecutionDomainState(
            std::shared_ptr<EngineState> engineState,
            wasmtime_store_t*            rawStore,
            wasmtime_linker_t*           rawLinker
        ):
            engine{ std::move(engineState) },
            store{ rawStore },
            linker{ rawLinker }
        {}

        void RegisterAsyncHostFunction(
            const std::string_view module,
            const std::string_view name
        )
        {
            asyncHostFunctions.emplace(MakeHostFunctionKey(module, name));
        }

        [[nodiscard]]
        bool HasAsyncHostFunction(
            const std::string_view module,
            const std::string_view name
        ) const
        {
            return asyncHostFunctions.find(MakeHostFunctionKey(module, name)) != asyncHostFunctions.end();
        }

        [[nodiscard]]
        bool UsesAsyncHostFunction(const ModuleMetadata& metadata)
        {
            for (const auto& import : metadata.imports)
            {
                if (import.kind != WASM_EXTERN_FUNC)
                {
                    continue;
                }

                if (HasAsyncHostFunction(import.module, import.name))
                {
                    return true;
                }
            }

            return false;
        }

    private:
        [[nodiscard]]
        static std::string MakeHostFunctionKey(
            const std::string_view module,
            const std::string_view name
        )
        {
            std::string key;
            key.reserve(module.size() + 1 + name.size());

            key.append(module.data(), module.size());
            key.push_back('\0');
            key.append(name.data(), name.size());

            return key;
        }
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
