# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picoRSSIScanTest C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# === Wi-Fi Scan Executable (background mode) ===
add_executable(picoRSSIScanTest
    picoRSSIScanTest.c
)

pico_set_program_name(picoRSSIScanTest "picoRSSIScanTest")
pico_set_program_version(picoRSSIScanTest "0.1")

# Enable USB stdio, disable UART stdio
pico_enable_stdio_uart(picoRSSIScanTest 0)
pico_enable_stdio_usb(picoRSSIScanTest 1)

# Add libraries (stdlib + Wi-Fi background driver)
target_link_libraries(picoRSSIScanTest
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_cyw43_arch_none
)


# Add include paths (needed for lwipopts.h)
target_include_directories(picoRSSIScanTest PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..    # for common lwipopts if you put it there
)

# Create output files (.uf2, .bin, etc.)
pico_add_extra_outputs(picoRSSIScanTest)
