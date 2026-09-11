#include "TestWat.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include <wasmtime.h>

namespace nexus::test
{
    namespace
    {
        std::string LoadFixture(const std::string_view fileName)
        {
            const std::string path = std::string{ NEXUSWASM_TEST_FIXTURE_DIR } + "/" + std::string{ fileName };

            std::ifstream file{ path, std::ios::binary };
            if (!file)
            {
                throw std::runtime_error{ "Failed to open WAT fixture: " + path };
            }

            return std::string{
                std::istreambuf_iterator<char>{ file },
                std::istreambuf_iterator<char>{}
            };
        }

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
    }

    std::vector<std::uint8_t> WatFileToWasm(const std::string_view fileName)
    {
        const std::string wat = LoadFixture(fileName);

        wasm_byte_vec_t   wasm{};
        wasmtime_error_t* error = wasmtime_wat2wasm(wat.data(), wat.size(), &wasm);

        if (error != nullptr)
        {
            throw std::runtime_error{
                "Failed to parse WAT fixture '" +
                std::string{ fileName } +
                "': " +
                TakeWasmtimeError(error)
            };
        }

        const auto* begin = reinterpret_cast<const std::uint8_t*>(wasm.data);

        std::vector<std::uint8_t> result{ begin, begin + wasm.size };

        wasm_byte_vec_delete(&wasm);

        return result;
    }
}
