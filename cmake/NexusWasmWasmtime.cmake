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

# ------------------------------------------------------------
# Default Wasmtime root
# ------------------------------------------------------------

set(
    _nexuswasm_default_wasmtime_root
    ""
)

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

# ------------------------------------------------------------
# Configure Wasmtime
# ------------------------------------------------------------

function(nexuswasm_configure_wasmtime)

    # --------------------------------------------------------
    # Already configured
    # --------------------------------------------------------

    if(TARGET NexusWasm::Wasmtime)

        return()

    endif()

    # --------------------------------------------------------
    # Normalize provider name
    # --------------------------------------------------------

    string(
        TOUPPER
        "${NEXUSWASM_WASMTIME_PROVIDER}"
        _provider
    )

    # ========================================================
    # PREBUILT provider
    # ========================================================

    if(_provider STREQUAL "PREBUILT")

        # ----------------------------------------------------
        # Root directory
        # ----------------------------------------------------

        if(NOT NEXUSWASM_WASMTIME_ROOT)

            message(
                FATAL_ERROR

                "NEXUSWASM_WASMTIME_PROVIDER=PREBUILT requires "
                "NEXUSWASM_WASMTIME_ROOT or the WASMTIME_C_API_ROOT "
                "environment variable."
            )

        endif()

        # ----------------------------------------------------
        # Include directory
        # ----------------------------------------------------

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

        # ====================================================
        # Validate exact Wasmtime version
        # ====================================================

        file(
            STRINGS

            "${_include_dir}/wasmtime.h"

            _version_line

            REGEX
                "^#define[ \t]+WASMTIME_VERSION[ \t]+\"[^\"]+\""
        )

        if(NOT _version_line)

            message(
                FATAL_ERROR

                "Could not determine Wasmtime version from: "
                "${_include_dir}/wasmtime.h"
            )

        endif()

        string(
            REGEX REPLACE

            "^#define[ \t]+WASMTIME_VERSION[ \t]+\"([^\"]+)\".*$"

            "\\1"

            _detected_version

            "${_version_line}"
        )

        if(
            NOT
            _detected_version
            STREQUAL
            NEXUSWASM_WASMTIME_VERSION
        )

            message(
                FATAL_ERROR

                "Wasmtime version mismatch. "
                "Expected ${NEXUSWASM_WASMTIME_VERSION}, "
                "found ${_detected_version}."
            )

        endif()

        # ====================================================
        # Validate native Wasmtime async support
        # ====================================================

        #
        # Wasmtime C API generates:
        #
        #   include/wasmtime/conf.h
        #
        # according to the features with which the C API itself
        # was built.
        #
        # We do NOT emulate missing async support.
        #

        set(
            _wasmtime_conf_header
            "${_include_dir}/wasmtime/conf.h"
        )

        if(NOT EXISTS "${_wasmtime_conf_header}")

            message(
                FATAL_ERROR

                "Wasmtime configuration header not found: "
                "${_wasmtime_conf_header}"
            )

        endif()

        # ----------------------------------------------------
        # async.h must exist
        # ----------------------------------------------------

        set(
            _wasmtime_async_header
            "${_include_dir}/wasmtime/async.h"
        )

        if(NOT EXISTS "${_wasmtime_async_header}")

            message(
                FATAL_ERROR

                "Wasmtime ${_detected_version} does not expose "
                "the native C async header required by NexusWasm: "
                "${_wasmtime_async_header}"
            )

        endif()

        # ----------------------------------------------------
        # Check WASMTIME_FEATURE_ASYNC
        # ----------------------------------------------------

        file(
            STRINGS

            "${_wasmtime_conf_header}"

            _wasmtime_async_feature_line

            REGEX
                "^[ \t]*#define[ \t]+WASMTIME_FEATURE_ASYNC([ \t]+.*)?$"
        )

        if(NOT _wasmtime_async_feature_line)

            message(
                FATAL_ERROR

                "The selected Wasmtime ${_detected_version} C API "
                "package was built without WASMTIME_FEATURE_ASYNC. "

                "NexusWasm Phase 07 requires Wasmtime native async "
                "execution support. "

                "No std::async, worker-thread, or other emulation "
                "will be used."
            )

        endif()

        # ====================================================
        # Locate static Wasmtime library
        # ====================================================

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

        # ====================================================
        # Imported Wasmtime target
        # ====================================================

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

        # ====================================================
        # Platform-specific configuration
        # ====================================================

        if(WIN32)

            #
            # Wasmtime's prebuilt static C API on Windows
            # must not import WASM/WASI symbols from a DLL.
            #

            target_compile_definitions(
                NexusWasmWasmtime

                INTERFACE

                "WASM_API_EXTERN="
                "WASI_API_EXTERN="
            )

            #
            # System libraries required by the static
            # Wasmtime C API on Windows.
            #

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

            find_package(
                Threads
                REQUIRED
            )

            target_link_libraries(
                NexusWasmWasmtime

                INTERFACE

                Threads::Threads
                ${CMAKE_DL_LIBS}
                m
            )

        endif()

        # ====================================================
        # Public alias used internally by NexusWasm
        # ====================================================

        add_library(
            NexusWasm::Wasmtime
            ALIAS
            NexusWasmWasmtime
        )

        # ====================================================
        # Configuration summary
        # ====================================================

        message(
            STATUS

            "NexusWasm Wasmtime: "
            "provider=PREBUILT "
            "version=${_detected_version} "
            "linkage=STATIC "
            "async=NATIVE "
            "root=${NEXUSWASM_WASMTIME_ROOT}"
        )

    # ========================================================
    # SYSTEM provider
    # ========================================================

    elseif(_provider STREQUAL "SYSTEM")

        message(
            FATAL_ERROR

            "SYSTEM Wasmtime provider is reserved but not implemented."
        )

    # ========================================================
    # BUNDLED provider
    # ========================================================

    elseif(_provider STREQUAL "BUNDLED")

        message(
            FATAL_ERROR

            "BUNDLED Wasmtime provider is reserved but not implemented."
        )

    # ========================================================
    # Unknown provider
    # ========================================================

    else()

        message(
            FATAL_ERROR

            "Unknown Wasmtime provider: "
            "${NEXUSWASM_WASMTIME_PROVIDER}"
        )

    endif()

endfunction()
