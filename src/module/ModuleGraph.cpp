#include "module/ModuleGraph.hpp"

#include <algorithm>
#include <functional>
#include <sstream>
#include <utility>

#include "module/ModuleMetadata.hpp"

#include "wasmtime/WasmtimeError.hpp"
#include "wasmtime/WasmtimeState.hpp"

namespace nexus::detail
{
    namespace
    {
        const char* ExternalKindName(const wasm_externkind_t kind) noexcept
        {
            switch (kind)
            {
            case WASM_EXTERN_FUNC:
                return "function";
            case WASM_EXTERN_GLOBAL:
                return "global";
            case WASM_EXTERN_TABLE:
                return "table";
            case WASM_EXTERN_MEMORY:
                return "memory";
            case WASM_EXTERN_TAG:
                return "tag";
            default:
                return "unknown";
            }
        }
    }

    Result<void> ModuleGraph::AddModule(
        const std::shared_ptr<EngineState>& engine,
        const std::string_view              moduleNamespace,
        const Module&                       module
    )
    {
        if (m_eLifecycle == Lifecycle::Instantiated)
        {
            return Error{
                ErrorCode::ModuleGraphAlreadyInstantiated,
                "Cannot add a module after the Program module graph has been instantiated."
            };
        }

        if (m_eLifecycle == Lifecycle::Failed)
        {
            return Error{
                ErrorCode::ModuleGraphFailed,
                "Cannot modify a failed Program module graph."
            };
        }

        if (m_eLifecycle != Lifecycle::Building)
        {
            return Error{
                ErrorCode::InvalidState,
                "Module graph is not currently mutable."
            };
        }

        if (moduleNamespace.empty())
        {
            return Error{
                ErrorCode::InvalidModuleNamespace,
                "Module namespace cannot be empty."
            };
        }

        if (!module.m_pState)
        {
            return Error{
                ErrorCode::InvalidState,
                "Module is in a moved-from state."
            };
        }

        if (module.m_pState->engine.get() != engine.get())
        {
            return Error{
                ErrorCode::RuntimeMismatch,
                "Module blongs to another Runtime."
            };
        }

        const std::string namespaceCopy{ moduleNamespace };

        if (m_mIndex.find(namespaceCopy) != m_mIndex.end())
        {
            return Error{
                ErrorCode::DuplicateModuleNamespace,
                "Duplicate Program module namespace: " + namespaceCopy
            };
        }

        const std::size_t index = m_vNodes.size();

        Node node{
            namespaceCopy,
            module,
            {},
            {},
            false
        };

        m_vNodes.push_back(std::move(node));
        m_mIndex.emplace(namespaceCopy, index);

        return {};
    }

    const ModuleExportInfo* ModuleGraph::FindExport(
        const Node&            node,
        const std::string_view name
    ) const
    {
        const auto& exports = node.module.m_pState->metadata.exports;

        for (const auto& current : exports)
        {
            if (current.name == name)
            {
                return &current;
            }
        }

        return nullptr;
    }

    Result<void> ModuleGraph::BuildDependencies()
    {
        for (auto& node : m_vNodes)
        {
            node.dependencies.clear();

            const auto& imports = node.module.m_pState->metadata.imports;
            for (const auto& import : imports)
            {
                const auto providerIt = m_mIndex.find(import.module);
                if (providerIt == m_mIndex.end())
                {
                    return Error{
                        ErrorCode::UnresolvedImport,
                        (
                            "Module '" +
                            node.moduleNamespace +
                            "' imports unresolved symbol '" +
                            import.module +
                            "." +
                            import.name +
                            "'."
                        )
                    };
                }

                const std::size_t providerIndex = providerIt->second;
                const Node&       provider = m_vNodes[providerIndex];

                if (import.kind != WASM_EXTERN_FUNC)
                {
                    return Error{
                        ErrorCode::UnsupportedDirectImportKind,
                        (
                            "Module '" +
                            node.moduleNamespace +
                            "' imports " +
                            ExternalKindName(import.kind) +
                            " '" +
                            import.module +
                            "." +
                            import.name +
                            "', but Phase 06 direct module linking "
                            "currently supports function imports only."
                        )
                    };
                }

                const ModuleExportInfo* exported = FindExport(provider, import.name);
                if (exported == nullptr)
                {
                    return Error{
                        ErrorCode::MissingDependencyExport,
                        (
                            "Module '" +
                            node.moduleNamespace +
                            "' requires '" +
                            import.module +
                            "." +
                            import.name +
                            "', but module '" +
                            import.module +
                            "' does not export that symbol."
                        )
                    };
                }

                if (exported->kind != WASM_EXTERN_FUNC)
                {
                    return Error{
                        ErrorCode::ImportKindMismatch,
                        (
                            "Module '" +
                            node.moduleNamespace +
                            "' requires function '" +
                            import.module +
                            "." +
                            import.name +
                            "', but the dependency exports a " +
                            ExternalKindName(exported->kind) +
                            "."
                        )
                    };
                }

                if (
                    IsSimpleFunctionSignature(import.functionSignature) &&
                    IsSimpleFunctionSignature(exported->functionSignature) &&
                    !FunctionSignatureEqual(import.functionSignature, exported->functionSignature)
                )
                {
                    return Error{
                        ErrorCode::ImportSignatureMismatch,
                        (
                            "Function signature mismatch for '" +
                            import.module +
                            "." +
                            import.name +
                            "'. Module '" +
                            node.moduleNamespace +
                            "' expects " +
                            FormatFunctionSignature(import.functionSignature) +
                            ", but module '" +
                            import.module +
                            "' exports " +
                            FormatFunctionSignature(exported->functionSignature) +
                            "."
                        )
                    };
                }

                if (std::find(
                    node.dependencies.begin(),
                    node.dependencies.end(),
                    providerIndex
                ) == node.dependencies.end())
                {
                    node.dependencies.push_back(providerIndex);
                }
            }
        }

        return {};
    }

    Result<std::vector<std::size_t>> ModuleGraph::BuildInstantiationOrder() const
    {
        enum class VisitState
        {
            Unvisited,
            Visiting,
            Visited
        };

        std::vector<VisitState>  states(m_vNodes.size(), VisitState::Unvisited);
        std::vector<std::size_t> stack;
        std::vector<std::size_t> order;

        order.reserve(m_vNodes.size());

        std::function<Result<void>(std::size_t)> visit;
        visit = [&](const std::size_t index) -> Result<void>
        {
            states[index] = VisitState::Visiting;

            stack.push_back(index);

            for (const std::size_t dependency : m_vNodes[index].dependencies)
            {
                if (states[dependency] == VisitState::Visiting)
                {
                    std::ostringstream message;

                    message << "Circular module dependency detected: ";

                    const auto cycleBegin = std::find(stack.begin(), stack.end(), dependency);
                    for (auto current = cycleBegin; current != stack.end(); ++current)
                    {
                        if (current != cycleBegin)
                        {
                            message << " -> ";
                        }

                        message << m_vNodes[*current].moduleNamespace;
                    }

                    message << " -> " << m_vNodes[dependency].moduleNamespace;

                    return Error{
                        ErrorCode::CyclicModuleDependency,
                        message.str()
                    };
                }

                if (states[dependency] == VisitState::Unvisited)
                {
                    auto result = visit(dependency);
                    if (!result)
                    {
                        return result.GetError();
                    }
                }
            }

            stack.pop_back();

            states[index] = VisitState::Visited;

            order.push_back(index);

            return {};
        };

        for (std::size_t i = 0; i < m_vNodes.size(); ++i)
        {
            if (states[i] != VisitState::Unvisited)
            {
                continue;
            }

            auto result = visit(i);
            if (!result)
            {
                return result.GetError();
            }
        }

        return order;
    }

    Result<void> ModuleGraph::Instantiate(const std::shared_ptr<ExecutionDomainState>& domain)
    {
        if (!domain)
        {
            return Error{
                ErrorCode::InvalidState,
                "ExecutionDomain is unavailable."
            };
        }

        if (m_eLifecycle == Lifecycle::Instantiated)
        {
            return Error{
                ErrorCode::ModuleGraphAlreadyInstantiated,
                "Program module graph has already been instantiated."
            };
        }

        if (m_eLifecycle == Lifecycle::Failed)
        {
            return Error{
                ErrorCode::ModuleGraphFailed,
                "Program module graph is in a failed state."
            };
        }

        if (m_eLifecycle != Lifecycle::Building)
        {
            return Error{
                ErrorCode::InvalidState,
                "Program module graph is currently being instantiated."
            };
        }

        if (m_vNodes.empty())
        {
            return Error{
                ErrorCode::ModuleGraphEmpty,
                "Program contains no modules."
            };
        }

        auto dependenciesResult = BuildDependencies();
        if (!dependenciesResult)
        {
            return dependenciesResult.GetError();
        }

        auto orderResult = BuildInstantiationOrder();
        if (!orderResult)
        {
            return orderResult.GetError();
        }

        const auto& order = orderResult.Value();

        m_eLifecycle = Lifecycle::Instantiating;

        wasmtime_context_t* context = wasmtime_store_context(domain->store.get());

        for (const std::size_t index : order)
        {
            Node& node = m_vNodes[index];

            wasmtime_instance_t rawInstance{};
            wasm_trap_t*        trap = nullptr;
            wasmtime_error_t*   error = wasmtime_linker_instantiate(
                domain->linker.get(),
                context,
                node.module.m_pState->module.get(),
                &rawInstance,
                &trap
            );

            if (error != nullptr)
            {
                m_eLifecycle = Lifecycle::Failed;
                return Error{
                    ErrorCode::InstantiationFailed,
                    (
                        "Failed to instantiate module '" +
                        node.moduleNamespace +
                        "': " +
                        TakeWasmtimeError(error)
                    )
                };
            }
            if (trap != nullptr)
            {
                m_eLifecycle = Lifecycle::Failed;
                return Error{
                    ErrorCode::Trap,
                    (
                        "Module '" +
                        node.moduleNamespace +
                        "' trapped during instantiation: " +
                        TakeWasmtimeTrap(trap)
                    )
                };
            }

            error = wasmtime_linker_define_instance(
                domain->linker.get(),
                context,
                node.moduleNamespace.data(),
                node.moduleNamespace.size(),
                &rawInstance
            );

            if (error != nullptr)
            {
                m_eLifecycle = Lifecycle::Failed;
                return Error{
                    ErrorCode::LinkerDefinitionFailed,
                    (
                        "Failed to publish module namespace '" +
                        node.moduleNamespace +
                        "' into the ExecutionDomain Linker: " +
                        TakeWasmtimeError(error)
                    )
                };
            }

            node.instance = rawInstance;
            node.hasInstance = true;
        }

        m_eLifecycle = Lifecycle::Instantiated;

        return {};
    }

    Result<Instance> ModuleGraph::GetInstance(
        const std::shared_ptr<ExecutionDomainState>& domain,
        const std::string_view                       moduleNamespace
    ) const
    {
        if (m_eLifecycle != Lifecycle::Instantiated)
        {
            return Error{
                ErrorCode::ModuleGraphNotInstantiated,
                "Program module graph has not been successfully instantiated."
            };
        }

        const auto it = m_mIndex.find(std::string{ moduleNamespace });
        if (it == m_mIndex.end())
        {
            return Error{
                ErrorCode::ModuleInstanceNotFound,
                (
                    "Program has no module instance named '" +
                    std::string{ moduleNamespace } +
                    "'."
                )
            };
        }

        const Node& node = m_vNodes[it->second];
        if (!node.hasInstance)
        {
            return Error{
                ErrorCode::InvalidState,
                "Module graph contains an uninstantiated node."
            };
        }

        auto instanceState = std::make_unique<InstanceState>(domain, node.instance);

        return Instance{ std::move(instanceState) };
    }
}
