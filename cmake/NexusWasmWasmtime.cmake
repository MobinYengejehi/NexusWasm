set(
    NEXUSWASM_WASMTIME_VERSION
    "48.0.1"
    CACHE STRING
    "Wasmtime version required by NexusWasm"
)

set(
    NEXUSWASM_WASMTIME_PROVIDER
    "PREBUILT"
    CACHE STRING
    "Wasmtime dependency provider"
)

set_property(
    CACHE NEXUSWASM_WASMTIME_PROVIDER
    PROPERTY STRINGS
        PREBUILT
        SYSTEM
        BUNDLED
)

set(_nexuswasm_default_wasmtime_root "")

if(DEFINED ENV{WASMTIME_C_API_ROOT})
    set(
        _nexuswasm_default_wasmtime_root
        "$ENV{WASMTIME_C_API_ROOT}"
    )
endif()

set(
    NEXUSWASM_WASMTIME_ROOT
    "${_nexuswasm_default_wasmtime_root}"
    CACHE PATH
    "Root directory of the Wasmtime C API package"
)

function(nexuswasm_configure_wasmtime)

    if(TARGET NexusWasm::Wasmtime)
        return()
    endif()

    string(
        TOUPPER
        "${NEXUSWASM_WASMTIME_PROVIDER}"
        _provider
    )

    if(_provider STREQUAL "PREBUILT")

        if(NOT NEXUSWASM_WASMTIME_ROOT)
            message(
                FATAL_ERROR
                "NEXUSWASM_WASMTIME_PROVIDER=PREBUILT requires "
                "NEXUSWASM_WASMTIME_ROOT or the WASMTIME_C_API_ROOT "
                "environment variable."
            )
        endif()

        set(
            _include_dir
            "${NEXUSWASM_WASMTIME_ROOT}/include"
        )

        if(NOT EXISTS "${_include_dir}/wasmtime.h")
            message(
                FATAL_ERROR
                "Wasmtime header not found: "
                "${_include_dir}/wasmtime.h"
            )
        endif()

        # --------------------------------------------------------
        # Validate exact Wasmtime version
        # --------------------------------------------------------

        file(
            STRINGS
            "${_include_dir}/wasmtime.h"
            _version_line
            REGEX "^#define[ \t]+WASMTIME_VERSION[ \t]+\"[^\"]+\""
        )

        if(NOT _version_line)
            message(
                FATAL_ERROR
                "Could not determine Wasmtime version from wasmtime.h"
            )
        endif()

        string(
            REGEX REPLACE
            "^#define[ \t]+WASMTIME_VERSION[ \t]+\"([^\"]+)\".*$"
            "\\1"
            _detected_version
            "${_version_line}"
        )

        if(NOT _detected_version STREQUAL NEXUSWASM_WASMTIME_VERSION)
            message(
                FATAL_ERROR
                "Wasmtime version mismatch. "
                "Expected ${NEXUSWASM_WASMTIME_VERSION}, "
                "found ${_detected_version}."
            )
        endif()

        # --------------------------------------------------------
        # Static Wasmtime
        # --------------------------------------------------------

        if(WIN32)

            set(
                _wasmtime_library
                "${NEXUSWASM_WASMTIME_ROOT}/lib/wasmtime.lib"
            )

        elseif(APPLE)

            set(
                _wasmtime_library
                "${NEXUSWASM_WASMTIME_ROOT}/lib/libwasmtime.a"
            )

        else()

            set(
                _wasmtime_library
                "${NEXUSWASM_WASMTIME_ROOT}/lib/libwasmtime.a"
            )

        endif()

        if(NOT EXISTS "${_wasmtime_library}")
            message(
                FATAL_ERROR
                "Wasmtime static library not found: "
                "${_wasmtime_library}"
            )
        endif()

        add_library(
            NexusWasmWasmtime
            STATIC
            IMPORTED
            GLOBAL
        )

        set_target_properties(
            NexusWasmWasmtime
            PROPERTIES

            IMPORTED_LOCATION
                "${_wasmtime_library}"

            INTERFACE_INCLUDE_DIRECTORIES
                "${_include_dir}"
        )

        if(WIN32)

            target_compile_definitions(
                NexusWasmWasmtime

                INTERFACE

                "WASM_API_EXTERN="
                "WASI_API_EXTERN="
            )

            target_link_libraries(
                NexusWasmWasmtime

                INTERFACE

                ws2_32
                advapi32
                userenv
                ntdll
                shell32
                ole32
                bcrypt
            )

        elseif(APPLE)

            target_link_libraries(
                NexusWasmWasmtime

                INTERFACE

                "-framework CoreFoundation"
            )

        else()

            find_package(Threads REQUIRED)

            target_link_libraries(
                NexusWasmWasmtime

                INTERFACE

                Threads::Threads
                ${CMAKE_DL_LIBS}
                m
            )

        endif()

        add_library(
            NexusWasm::Wasmtime
            ALIAS
            NexusWasmWasmtime
        )

        message(
            STATUS
            "NexusWasm Wasmtime: "
            "provider=PREBUILT "
            "version=${_detected_version} "
            "linkage=STATIC "
            "root=${NEXUSWASM_WASMTIME_ROOT}"
        )

    elseif(_provider STREQUAL "SYSTEM")

        message(
            FATAL_ERROR
            "SYSTEM Wasmtime provider is reserved but not implemented "
            "in milestone 03."
        )

    elseif(_provider STREQUAL "BUNDLED")

        message(
            FATAL_ERROR
            "BUNDLED Wasmtime provider is reserved but not implemented "
            "in milestone 03."
        )

    else()

        message(
            FATAL_ERROR
            "Unknown Wasmtime provider: "
            "${NEXUSWASM_WASMTIME_PROVIDER}"
        )

    endif()

endfunction()
