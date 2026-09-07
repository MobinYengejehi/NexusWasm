add_executable(
    nexuswasm_example_basic
    basic/main.cpp
)

target_link_libraries(
    nexuswasm_example_basic
    PRIVATE
    NexusWasm::NexusWasm
)

set_target_properties(
    nexuswasm_example_basic
    PROPERTIES

    CXX_EXTENSIONS OFF

    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
)

nexuswasm_enable_warnings(nexuswasm_example_basic)
