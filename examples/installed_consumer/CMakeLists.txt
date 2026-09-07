cmake_minimum_required(VERSION 3.23)

project(
    NexusWasmInstalledConsumer
    LANGUAGES CXX
)

find_package(
    NexusWasm
    CONFIG
    REQUIRED
)

add_executable(
    nexuswasm_installed_consumer
    main.cpp
)

target_link_libraries(
    nexuswasm_installed_consumer
    PRIVATE
    NexusWasm::NexusWasm
)
