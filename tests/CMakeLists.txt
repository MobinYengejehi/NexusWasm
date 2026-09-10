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
