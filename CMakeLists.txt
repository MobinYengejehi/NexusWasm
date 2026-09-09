cmake_minimum_required(VERSION 3.23...4.4)

project(
    NexusWasm
    VERSION 0.1.0
    DESCRIPTION "General-purpose embeddable WebAssembly runtime framework for C++"
    LANGUAGES CXX
)

# ------------------------------------------------------------
# CMake modules
# ------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

# ------------------------------------------------------------
# Options
# ------------------------------------------------------------

option(
    BUILD_SHARED_LIBS
    "Build NexusWasm as a shared library"
    OFF
)

option(
    NEXUSWASM_BUILD_TESTS
    "Build NexusWasm tests"
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    NEXUSWASM_BUILD_EXAMPLES
    "Build NexusWasm examples"
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    NEXUSWASM_BUILD_BENCHMARKS
    "Build NexusWasm benchmarks"
    OFF
)

option(
    NEXUSWASM_WARNINGS_AS_ERRORS
    "Treat NexusWasm compiler warnings as errors"
    OFF
)

# ------------------------------------------------------------
# Project CMake modules
# ------------------------------------------------------------

list(
    APPEND
    CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

include(NexusWasmWarnings)
include(NexusWasmWasmtime)

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------

nexuswasm_configure_wasmtime()

# ------------------------------------------------------------
# NexusWasm library
# ------------------------------------------------------------

add_library(
    NexusWasm

    src/Version.cpp
    src/Runtime.cpp
    src/CompiledModule.cpp
    src/Instance.cpp
)

add_library(
    NexusWasm::NexusWasm
    ALIAS
    NexusWasm
)

# ------------------------------------------------------------
# Private implementation dependencies
# ------------------------------------------------------------

#
# Wasmtime is an implementation detail of NexusWasm.
#
# Runtime.cpp / Instance.cpp / WasmtimeState.hpp may include Wasmtime,
# but no public NexusWasm header may expose Wasmtime types.
#
# BUILD_INTERFACE is intentional for this milestone:
# it keeps the local PREBUILT SDK path out of the installed package.
#
# A complete installed-static Wasmtime dependency policy will be added
# when the provider/package strategy is finalized.
#

target_link_libraries(
    NexusWasm

    PRIVATE

    $<BUILD_INTERFACE:NexusWasm::Wasmtime>
)

# Allow implementation files to include:
#
#   #include "wasmtime/WasmtimeState.hpp"
#

target_include_directories(
    NexusWasm

    PRIVATE

    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# ------------------------------------------------------------
# Generated public export header
# ------------------------------------------------------------

set(
    NEXUSWASM_GENERATED_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated/include"
)

file(
    MAKE_DIRECTORY
    "${NEXUSWASM_GENERATED_INCLUDE_DIR}/nexuswasm"
)

generate_export_header(
    NexusWasm

    EXPORT_FILE_NAME
        "${NEXUSWASM_GENERATED_INCLUDE_DIR}/nexuswasm/Export.hpp"

    EXPORT_MACRO_NAME
        NEXUSWASM_API

    NO_EXPORT_MACRO_NAME
        NEXUSWASM_NO_EXPORT

    DEPRECATED_MACRO_NAME
        NEXUSWASM_DEPRECATED

    STATIC_DEFINE
        NEXUSWASM_STATIC_DEFINE
)

# ------------------------------------------------------------
# Public headers
# ------------------------------------------------------------

target_sources(
    NexusWasm

    PUBLIC

    FILE_SET public_headers
    TYPE HEADERS

    BASE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${NEXUSWASM_GENERATED_INCLUDE_DIR}"

    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexuswasm/Error.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexuswasm/Result.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexuswasm/Runtime.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexuswasm/CompiledModule.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexuswasm/Instance.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexuswasm/Version.hpp"

        "${NEXUSWASM_GENERATED_INCLUDE_DIR}/nexuswasm/Export.hpp"
)

# ------------------------------------------------------------
# Include directories
# ------------------------------------------------------------

target_include_directories(
    NexusWasm

    PUBLIC

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${NEXUSWASM_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ------------------------------------------------------------
# C++ requirements
# ------------------------------------------------------------

target_compile_features(
    NexusWasm

    PUBLIC

    cxx_std_17
)

# ------------------------------------------------------------
# Target properties
# ------------------------------------------------------------

set_target_properties(
    NexusWasm

    PROPERTIES

    CXX_EXTENSIONS OFF

    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"

    LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/lib"

    ARCHIVE_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/lib"
)

# ------------------------------------------------------------
# Static library export handling
# ------------------------------------------------------------

if(NOT BUILD_SHARED_LIBS)

    target_compile_definitions(
        NexusWasm

        PUBLIC

        NEXUSWASM_STATIC_DEFINE
    )

endif()

# ------------------------------------------------------------
# NexusWasm version information
# ------------------------------------------------------------

target_compile_definitions(
    NexusWasm

    PRIVATE

    NEXUSWASM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NEXUSWASM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NEXUSWASM_VERSION_PATCH=${PROJECT_VERSION_PATCH}

    NEXUSWASM_VERSION_STRING="${PROJECT_VERSION}"
)

# ------------------------------------------------------------
# Compiler warnings
# ------------------------------------------------------------

nexuswasm_enable_warnings(
    NexusWasm
)

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

if(NEXUSWASM_BUILD_TESTS)

    enable_testing()

    add_subdirectory(
        tests
    )

endif()

# ------------------------------------------------------------
# Examples
# ------------------------------------------------------------

if(NEXUSWASM_BUILD_EXAMPLES)

    add_subdirectory(
        examples
    )

endif()

# ------------------------------------------------------------
# Benchmarks
# ------------------------------------------------------------

if(NEXUSWASM_BUILD_BENCHMARKS)

    add_subdirectory(
        benchmarks
    )

endif()

# ------------------------------------------------------------
# Installation
# ------------------------------------------------------------

set(
    NEXUSWASM_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/NexusWasm"
)

install(
    TARGETS NexusWasm

    EXPORT NexusWasmTargets

    RUNTIME
        DESTINATION "${CMAKE_INSTALL_BINDIR}"

    LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"

    ARCHIVE
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"

    FILE_SET public_headers
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# ------------------------------------------------------------
# Exported CMake targets
# ------------------------------------------------------------

install(
    EXPORT NexusWasmTargets

    FILE
        NexusWasmTargets.cmake

    NAMESPACE
        NexusWasm::

    DESTINATION
        "${NEXUSWASM_INSTALL_CMAKEDIR}"
)

# ------------------------------------------------------------
# Package configuration
# ------------------------------------------------------------

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusWasmConfig.cmake.in"

    "${CMAKE_CURRENT_BINARY_DIR}/NexusWasmConfig.cmake"

    INSTALL_DESTINATION
        "${NEXUSWASM_INSTALL_CMAKEDIR}"
)

# NexusWasm 0.x:
#
# 0.1.x is considered one compatibility line.
# 0.2.x may contain breaking API changes.

if(PROJECT_VERSION_MAJOR EQUAL 0)

    set(
        NEXUSWASM_VERSION_COMPATIBILITY
        SameMinorVersion
    )

else()

    set(
        NEXUSWASM_VERSION_COMPATIBILITY
        SameMajorVersion
    )

endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NexusWasmConfigVersion.cmake"

    VERSION
        "${PROJECT_VERSION}"

    COMPATIBILITY
        "${NEXUSWASM_VERSION_COMPATIBILITY}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/NexusWasmConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/NexusWasmConfigVersion.cmake"

    DESTINATION
        "${NEXUSWASM_INSTALL_CMAKEDIR}"
)
