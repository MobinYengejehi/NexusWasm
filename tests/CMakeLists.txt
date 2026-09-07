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

set_target_properties(
    nexuswasm_version_tests
    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(nexuswasm_version_tests)

add_test(
    NAME
        nexuswasm.version

    COMMAND
        nexuswasm_version_tests
)

add_executable(
    nexuswasm_wasmtime_tests

    WasmtimeTests.cpp

    $<TARGET_OBJECTS:nexuswasm_wasmtime_objects>
)

target_include_directories(
    nexuswasm_wasmtime_tests

    PRIVATE

    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(
    nexuswasm_wasmtime_tests

    PRIVATE

    NexusWasm::Wasmtime
)

target_compile_features(
    nexuswasm_wasmtime_tests

    PRIVATE

    cxx_std_17
)

target_compile_definitions(
    nexuswasm_wasmtime_tests

    PRIVATE

    NEXUSWASM_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

set_target_properties(
    nexuswasm_wasmtime_tests

    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(
    nexuswasm_wasmtime_tests
)

add_test(
    NAME
        nexuswasm.wasmtime

    COMMAND
        nexuswasm_wasmtime_tests
)
