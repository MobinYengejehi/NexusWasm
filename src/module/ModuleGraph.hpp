#pragma once

#ifndef NEXUSWASM_MODULE_GRAPH_HEADER
#define NEXUSWASM_MODULE_GRAPH_HEADER

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <wasmtime.h>

#include <nexuswasm/Instance.hpp>
#include <nexuswasm/Module.hpp>
#include <nexuswasm/Result.hpp>

#include "module/ModuleMetadata.hpp"

namespace nexus::detail
{
    struct EngineState;
    struct ExecutionDomainState;

    class ModuleGraph final
    {
    public:
        [[nodiscard]]
        Result<void> AddModule(
            const std::shared_ptr<EngineState>& engine,
            std::string_view                    moduleNamespace,
            const Module&                       module
        );

        [[nodiscard]]
        Result<void> Instantiate(const std::shared_ptr<ExecutionDomainState>& domain);

        [[nodiscard]]
        Result<Instance> GetInstance(
            const std::shared_ptr<ExecutionDomainState>& domain,
            const std::string_view                       moduleNamespace
        ) const;

    private:
        enum class Lifecycle
        {
            Building,
            Instantiating,
            Instantiated,
            Failed
        };

        struct Node final
        {
            std::string moduleNamespace;

            Module module;

            std::vector<std::size_t> dependencies;

            wasmtime_instance_t instance{};
            bool                hasInstance = false;
        };

        Lifecycle m_eLifecycle = Lifecycle::Building;

        std::vector<Node> m_vNodes;

        std::unordered_map<std::string, std::size_t> m_mIndex;

        [[nodiscard]]
        Result<void> BuildDependencies();

        [[nodiscard]]
        Result<std::vector<std::size_t>> BuildInstantiationOrder() const;

        [[nodiscard]]
        const ModuleExportInfo* FindExport(const Node& node, std::string_view name) const;
    };
}

#endif
