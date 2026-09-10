#include "wasmtime/WasmtimeError.hpp"

namespace nexus::detail
{
    std::string TakeWasmtimeError(wasmtime_error_t* error)
    {
        if (error == nullptr)
        {
            return {};
        }

        wasm_name_t message;
        wasmtime_error_message(error, &message);

        std::string result{ message.data, message.size };

        wasm_byte_vec_delete(&message);
        wasmtime_error_delete(error);

        if (!result.empty() && result.back() == '\0')
        {
            result.pop_back();
        }

        return result;
    }

    std::string TakeWasmtimeTrap(wasm_trap_t* trap)
    {
        if (trap == nullptr)
        {
            return {};
        }

        wasm_message_t message{};
        wasm_trap_message(trap, &message);

        std::string result{ message.data, message.size };

        wasm_byte_vec_delete(&message);
        wasm_trap_delete(trap);

        if (!result.empty() && result.back() == '\0')
        {
            result.pop_back();
        }

        return result;
    }
}
