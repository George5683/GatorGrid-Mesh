# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picow_access_point C CXX ASM)

option(APP_DEBUG "Enable app-level debug paths" ON)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
file(GLOB_RECURSE SRC_FILES 
     "${CMAKE_CURRENT_SOURCE_DIR}/libraries/*/*.c" 
     "${CMAKE_CURRENT_SOURCE_DIR}/libraries/*/*.cpp")

file(GLOB LIBRARY_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/libraries/*")

# ________________________________________ Cmake for sta_background ________________________________________
add_executable(sta_background
        Tests/sta_test.cpp
        ${SRC_FILES}
        )

target_include_directories(sta_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(sta_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )


if (APP_DEBUG)
  target_compile_definitions(sta_background PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(sta_background 0)
pico_enable_stdio_usb(sta_background 1)
        

pico_add_extra_outputs(sta_background)

# ________________________________________ Cmake for ap_background ________________________________________
add_executable(ap_background
        Tests/AP_Test.cpp
        ${SRC_FILES}
        )

target_include_directories(ap_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(ap_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(ap_background PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(ap_background 0)
pico_enable_stdio_usb(ap_background 1)
        

pico_add_extra_outputs(ap_background)

# ________________________________________ Cmake for sta_root ________________________________________
add_executable(sta_root
        Tests/sta_root.cpp
        ${SRC_FILES}
        )

target_include_directories(sta_root PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(sta_root
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(sta_root PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(sta_root 0)
pico_enable_stdio_usb(sta_root 1)
        

pico_add_extra_outputs(sta_root)

# ________________________________________ Cmake for ap_root ________________________________________
add_executable(ap_root
        Tests/ap_root.cpp
        ${SRC_FILES}
        )

target_include_directories(ap_root PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(ap_root
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(ap_root PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(ap_root 0)
pico_enable_stdio_usb(ap_root 1)
        

pico_add_extra_outputs(ap_root)

# node 1
# ________________________________________ Cmake for sta_one ________________________________________
add_executable(sta_one
        Tests/sta_one.cpp
        ${SRC_FILES}
        )

target_include_directories(sta_one PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(sta_one
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(sta_one PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(sta_one 0)
pico_enable_stdio_usb(sta_one 1)
        

pico_add_extra_outputs(sta_one)

# ________________________________________ Cmake for ap_one ________________________________________

add_executable(ap_one
        Tests/ap_one.cpp
        ${SRC_FILES}
        )

target_include_directories(ap_one PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(ap_one
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(ap_one PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(ap_one 0)
pico_enable_stdio_usb(ap_one 1)
        

pico_add_extra_outputs(ap_one)

# node 2
# ________________________________________ Cmake for sta_two ________________________________________
add_executable(sta_two
        Tests/sta_two.cpp
        ${SRC_FILES}
        )

target_include_directories(sta_two PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(sta_two
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(sta_two PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(sta_two 0)
pico_enable_stdio_usb(sta_two 1)
        

pico_add_extra_outputs(sta_two)

# ________________________________________ Cmake for ap_two ________________________________________

add_executable(ap_two
        Tests/ap_two.cpp
        ${SRC_FILES}
        )

target_include_directories(ap_two PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(ap_two
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        )

if (APP_DEBUG)
  target_compile_definitions(ap_two PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(ap_two 0)
pico_enable_stdio_usb(ap_two 1)
        

pico_add_extra_outputs(ap_two)

# node 3
# ________________________________________ Cmake for sta_three ________________________________________
add_executable(sta_three
        Tests/sta_three.cpp
        ${SRC_FILES}
        )

target_include_directories(sta_three PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(sta_three
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(sta_three PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(sta_three 0)
pico_enable_stdio_usb(sta_three 1)
        

pico_add_extra_outputs(sta_three)

# ________________________________________ Cmake for ap_three ________________________________________

add_executable(ap_three
        Tests/ap_three.cpp
        ${SRC_FILES}
        )

target_include_directories(ap_three PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(ap_three
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(ap_three PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(ap_three 0)
pico_enable_stdio_usb(ap_three 1)
        

pico_add_extra_outputs(ap_three)

# node 4
# ________________________________________ Cmake for sta_four ________________________________________
add_executable(sta_four
        Tests/sta_four.cpp
        ${SRC_FILES}
        )

target_include_directories(sta_four PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(sta_four
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(sta_four PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(sta_four 0)
pico_enable_stdio_usb(sta_four 1)
        

pico_add_extra_outputs(sta_four)

# ________________________________________ Cmake for ap_four ________________________________________

add_executable(ap_four
        Tests/ap_four.cpp
        ${SRC_FILES}
        )

target_include_directories(ap_four PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(ap_four
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(ap_four PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(ap_four 0)
pico_enable_stdio_usb(ap_four 1)
        

pico_add_extra_outputs(ap_four)

# node 5
# ________________________________________ Cmake for sta_five ________________________________________
add_executable(sta_five
        Tests/sta_five.cpp
        ${SRC_FILES}
        )

target_include_directories(sta_five PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(sta_five
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(sta_five PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(sta_five 0)
pico_enable_stdio_usb(sta_five 1)
        

pico_add_extra_outputs(sta_five)

# ________________________________________ Cmake for ap_five ________________________________________

add_executable(ap_five
        Tests/ap_five.cpp
        ${SRC_FILES}
        )

target_include_directories(ap_five PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${LIBRARY_DIRS}
        )

target_link_libraries(ap_five
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

if (APP_DEBUG)
  target_compile_definitions(ap_five PRIVATE DEBUG=1)
endif()

pico_enable_stdio_uart(ap_five 0)
pico_enable_stdio_usb(ap_five 1)
        

pico_add_extra_outputs(ap_five)