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
# Core public API tests
# ------------------------------------------------------------

add_executable(
    nexuswasm_core_api_tests

    CoreApiTests.cpp
)

target_link_libraries(
    nexuswasm_core_api_tests

    PRIVATE

    NexusWasm::NexusWasm
)

target_compile_features(
    nexuswasm_core_api_tests

    PRIVATE

    cxx_std_17
)

set_target_properties(
    nexuswasm_core_api_tests

    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(
    nexuswasm_core_api_tests
)

add_test(
    NAME
        nexuswasm.core_api

    COMMAND
        nexuswasm_core_api_tests
)
