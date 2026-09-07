cmake_minimum_required(VERSION 3.23...4.4)

project(
    NexusWasm
    VERSION 0.1.0
    DESCRIPTION "General-purpose embeddable WebAssembly runtime framework for C++"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

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

list(
    APPEND
    CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

include(NexusWasmWarnings)
include(NexusWasmWasmtime)

nexuswasm_configure_wasmtime()

# ------------------------------------------------------------
# Library
# ------------------------------------------------------------

add_library(
    NexusWasm
    src/Version.cpp
)

add_library(
    NexusWasm::NexusWasm
    ALIAS
    NexusWasm
)

add_library(
    nexuswasm_wasmtime_objects
    OBJECT

    src/wasmtime/WasmtimeProbe.cpp
)

target_include_directories(
    nexuswasm_wasmtime_objects
    PRIVATE

    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(
    nexuswasm_wasmtime_objects
    PRIVATE

    cxx_std_17
)

target_link_libraries(
    nexuswasm_wasmtime_objects
    PRIVATE

    NexusWasm::Wasmtime
)

set_target_properties(
    nexuswasm_wasmtime_objects
    PROPERTIES

    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

nexuswasm_enable_warnings(
    nexuswasm_wasmtime_objects
)

target_sources(
    NexusWasm
    PRIVATE

    $<TARGET_OBJECTS:nexuswasm_wasmtime_objects>
)

if (BUILD_SHARED_LIBS)
    target_link_libraries(
        NexusWasm
        PRIVATE

        NexusWasm::Wasmtime
    )
endif()

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
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexuswasm/Version.hpp"
        "${NEXUSWASM_GENERATED_INCLUDE_DIR}/nexuswasm/Export.hpp"
)

target_include_directories(
    NexusWasm

    PUBLIC

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${NEXUSWASM_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Public requirement: NexusWasm requires at least C++17.
target_compile_features(
    NexusWasm
    PUBLIC
    cxx_std_17
)

set_target_properties(
    NexusWasm
    PROPERTIES

    CXX_EXTENSIONS OFF

    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES

    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# GenerateExportHeader needs consumers of a static library
# to see this definition too.
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(
        NexusWasm
        PUBLIC
        NEXUSWASM_STATIC_DEFINE
    )
endif()

# Keep project version data private to implementation.
target_compile_definitions(
    NexusWasm

    PRIVATE

    NEXUSWASM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NEXUSWASM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NEXUSWASM_VERSION_PATCH=${PROJECT_VERSION_PATCH}

    NEXUSWASM_VERSION_STRING="${PROJECT_VERSION}"
)

nexuswasm_enable_warnings(NexusWasm)

# ------------------------------------------------------------
# Tests / Examples / Benchmarks
# ------------------------------------------------------------

if(NEXUSWASM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NEXUSWASM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(NEXUSWASM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
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

# NexusWasm is still 0.x.
# Treat 0.<minor>.x as one compatibility line.
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(NEXUSWASM_VERSION_COMPATIBILITY SameMinorVersion)
else()
    set(NEXUSWASM_VERSION_COMPATIBILITY SameMajorVersion)
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
