#include "module/ModuleMetadata.hpp"

#include <sstream>

namespace nexus::detail
{
    namespace
    {
        std::string NameToString(const wasm_name_t* name)
        {
            if (name == nullptr)
            {
                return {};
            }

            return std::string{ name->data, name->size };
        }

        FunctionSignature ReadFunctionSignature(const wasm_externtype_t* type)
        {
            FunctionSignature result;

            if (type == nullptr)
            {
                return result;
            }

            const wasm_functype_t* functionType = wasm_externtype_as_functype_const(type);
            if (functionType == nullptr)
            {
                return result;
            }

            const wasm_valtype_vec_t* parameters = wasm_functype_params(functionType);
            const wasm_valtype_vec_t* results = wasm_functype_results(functionType);

            result.parameters.reserve(parameters->size);

            for (std::size_t i = 0; i < parameters->size; ++i)
            {
                result.parameters.push_back(wasm_valtype_kind(parameters->data[i]));
            }

            result.results.reserve(results->size);

            for (std::size_t i = 0; i < results->size; ++i)
            {
                result.results.push_back(wasm_valtype_kind(results->data[i]));
            }

            return result;
        }

        const char* ValueKindName(const wasm_valkind_t kind)
        {
            switch (kind)
            {
            case WASM_I32:
                return "i32";
            case WASM_I64:
                return "i64";
            case WASM_F32:
                return "f32";
            case WASM_F64:
                return "f64";
            case WASM_EXTERNREF:
                return "externref";
            case WASM_FUNCREF:
                return "funcref";
            default:
                return "unknown";
            }
        }

        bool IsSimpleValueKind(const wasm_valkind_t kind) noexcept
        {
            return (
                kind == WASM_I32 ||
                kind == WASM_I64 ||
                kind == WASM_F32 ||
                kind == WASM_F64
            );
        }
    }

    ModuleMetadata InspectModule(const wasmtime_module_t* module)
    {
        ModuleMetadata metadata;

        wasm_importtype_vec_t imports{};
        wasmtime_module_imports(module, &imports);

        struct ImportVectorGuard final
        {
            wasm_importtype_vec_t* value;

            ~ImportVectorGuard()
            {
                wasm_importtype_vec_delete(value);
            }
        };

        ImportVectorGuard importGuard{ &imports };

        metadata.imports.reserve(imports.size);

        for (std::size_t i = 0; i < imports.size; ++i)
        {
            const wasm_importtype_t* importType = imports.data[i];
            const wasm_externtype_t* externalType = wasm_importtype_type(importType);

            ModuleImportInfo info;
            info.module = NameToString(wasm_importtype_module(importType));
            info.name = NameToString(wasm_importtype_name(importType));
            info.kind = wasm_externtype_kind(externalType);

            if (info.kind == WASM_EXTERN_FUNC)
            {
                info.functionSignature = ReadFunctionSignature(externalType);
            }

            metadata.imports.push_back(std::move(info));
        }

        wasm_exporttype_vec_t exports{};
        wasmtime_module_exports(module, &exports);

        struct ExportVectorGuard final
        {
            wasm_exporttype_vec_t* value;

            ~ExportVectorGuard()
            {
                wasm_exporttype_vec_delete(value);
            }
        };

        ExportVectorGuard exportGuard{ &exports };

        metadata.exports.reserve(exports.size);

        for (std::size_t i = 0; i < exports.size; ++i)
        {
            const wasm_exporttype_t* exportType = exports.data[i];
            const wasm_externtype_t* externalType = wasm_exporttype_type(exportType);

            ModuleExportInfo info;
            info.name = NameToString(wasm_exporttype_name(exportType));
            info.kind = wasm_externtype_kind(externalType);

            if (info.kind == WASM_EXTERN_FUNC)
            {
                info.functionSignature = ReadFunctionSignature(externalType);
            }

            metadata.exports.push_back(std::move(info));
        }

        return metadata;
    }

    bool IsSimpleFunctionSignature(const FunctionSignature& signature) noexcept
    {
        for (const auto kind : signature.parameters)
        {
            if (!IsSimpleValueKind(kind))
            {
                return false;
            }
        }

        for (const auto kind : signature.results)
        {
            if (!IsSimpleValueKind(kind))
            {
                return false;
            }
        }

        return true;
    }

    bool FunctionSignatureEqual(
        const FunctionSignature& lhs,
        const FunctionSignature& rhs
    ) noexcept
    {
        return (
            lhs.parameters == rhs.parameters &&
            lhs.results == rhs.results
        );
    }

    std::string FormatFunctionSignature(const FunctionSignature& signature)
    {
        std::ostringstream stream;

        stream << '(';

        for (std::size_t i = 0; i < signature.parameters.size(); ++i)
        {
            if (i != 0)
            {
                stream << ", ";
            }

            stream << ValueKindName(signature.parameters[i]);
        }

        stream << ") -> (";

        for (std::size_t i = 0; i < signature.results.size(); ++i)
        {
            if (i != 0)
            {
                stream << ", ";
            }

            stream << ValueKindName(signature.results[i]);
        }

        stream << ')';

        return stream.str();
    }
}
