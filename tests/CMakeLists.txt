# ------------------------------------------------------------
# Version tests
# ------------------------------------------------------------

add_executable(
    nexuswasm_version_tests

    VersionTests.cpp
)

target_link_libraries(
    nexuswasm_version_tests

    PRIVATE

    NexusWasm::NexusWasm
)

target_compile_definitions(
    nexuswasm_version_tests

    PRIVATE

    NEXUSWASM_EXPECTED_VERSION="${PROJECT_VERSION}"

    NEXUSWASM_EXPECTED_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NEXUSWASM_EXPECTED_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NEXUSWASM_EXPECTED_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_compile_features(
    nexuswasm_version_tests

    PRIVATE

    cxx_std_17
)

set_target_properties(
    nexuswasm_version_tests

    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(
    nexuswasm_version_tests
)

add_test(
    NAME
        nexuswasm.version

    COMMAND
        nexuswasm_version_tests
)

# ------------------------------------------------------------
# Runtime architecture tests
# ------------------------------------------------------------

add_executable(
    nexuswasm_runtime_architecture_tests

    RuntimeArchitectureTests.cpp
)

target_link_libraries(
    nexuswasm_runtime_architecture_tests

    PRIVATE

    NexusWasm::NexusWasm
)

target_compile_features(
    nexuswasm_runtime_architecture_tests

    PRIVATE

    cxx_std_17
)

set_target_properties(
    nexuswasm_runtime_architecture_tests

    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(
    nexuswasm_runtime_architecture_tests
)

add_test(
    NAME
        nexuswasm.runtime_architecture

    COMMAND
        nexuswasm_runtime_architecture_tests
)

# ------------------------------------------------------------
# Module graph tests
# ------------------------------------------------------------

add_executable(
    nexuswasm_module_graph_tests

    ModuleGraphTests.cpp
    TestWat.cpp
)

target_include_directories(
    nexuswasm_module_graph_tests

    PRIVATE

    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(
    nexuswasm_module_graph_tests
    PRIVATE
    NEXUSWASM_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/module_graph"
)

target_link_libraries(
    nexuswasm_module_graph_tests

    PRIVATE

    NexusWasm::NexusWasm

    # Used only by TestWat.cpp for wasmtime_wat2wasm().
    NexusWasm::Wasmtime
)

target_compile_features(
    nexuswasm_module_graph_tests

    PRIVATE

    cxx_std_17
)

set_target_properties(
    nexuswasm_module_graph_tests

    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(
    nexuswasm_module_graph_tests
)

add_test(
    NAME
        nexuswasm.module_graph

    COMMAND
        nexuswasm_module_graph_tests
)

# ------------------------------------------------------------
# Native async tests
# ------------------------------------------------------------

add_executable(
    nexuswasm_async_tests

    AsyncTests.cpp
    TestWat.cpp
)

target_include_directories(
    nexuswasm_async_tests

    PRIVATE

    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(
    nexuswasm_async_tests

    PRIVATE

    NEXUSWASM_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/async"
)

target_link_libraries(
    nexuswasm_async_tests

    PRIVATE

    NexusWasm::NexusWasm

    # TestWat.cpp uses wasmtime_wat2wasm().
    NexusWasm::Wasmtime
)

target_compile_features(
    nexuswasm_async_tests

    PRIVATE

    cxx_std_17
)

set_target_properties(
    nexuswasm_async_tests

    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(
    nexuswasm_async_tests
)

add_test(
    NAME
        nexuswasm.native_async

    COMMAND
        nexuswasm_async_tests
)
