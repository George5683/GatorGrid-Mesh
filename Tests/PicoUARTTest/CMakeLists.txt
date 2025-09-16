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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PicoUARTMulti C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# =======================================================
# First executable: PicoUARTHerobrineTest
add_executable(PicoUARTHerobrineTest PicoUARTHerobrineTest.c)

pico_set_program_name(PicoUARTHerobrineTest "PicoUARTHerobrineTest")
pico_set_program_version(PicoUARTHerobrineTest "0.1")

pico_enable_stdio_uart(PicoUARTHerobrineTest 0)
pico_enable_stdio_usb(PicoUARTHerobrineTest 1)

target_link_libraries(PicoUARTHerobrineTest pico_stdlib hardware_uart)

target_include_directories(PicoUARTHerobrineTest PRIVATE
        ${CMAKE_CURRENT_LIST_DIR})

pico_add_extra_outputs(PicoUARTHerobrineTest)

# =======================================================
# Second executable: PicoUARTFortniterTest
add_executable(PicoUARTFortniterTest PicoUARTFortniterTest.c)

pico_set_program_name(PicoUARTFortniterTest "PicoUARTFortniterTest")
pico_set_program_version(PicoUARTFortniterTest "0.1")

pico_enable_stdio_uart(PicoUARTFortniterTest 0)
pico_enable_stdio_usb(PicoUARTFortniterTest 1)

target_link_libraries(PicoUARTFortniterTest pico_stdlib hardware_uart)

target_include_directories(PicoUARTFortniterTest PRIVATE
        ${CMAKE_CURRENT_LIST_DIR})

pico_add_extra_outputs(PicoUARTFortniterTest)
