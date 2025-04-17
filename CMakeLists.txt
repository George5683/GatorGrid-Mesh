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
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picow_access_point C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

# Cmake for sta_testing
add_executable(sta_background
        Tests/sta_test.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        libraries/MeshNode/Messages.cpp
        libraries/SPI/SPI.cpp
        libraries/RingBuffer/RingBuffer.cpp
        libraries/ChildrenTree/ChildrenTree.cpp
        )

target_include_directories(sta_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
        ${CMAKE_CURRENT_LIST_DIR}/libraries/RingBuffer
        ${CMAKE_CURRENT_LIST_DIR}/libraries/ChildrenTree
        )

target_link_libraries(sta_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        )

pico_enable_stdio_uart(sta_background 0)
pico_enable_stdio_usb(sta_background 1)
        

pico_add_extra_outputs(sta_background)

# Cmake for AP_testing
add_executable(ap_background
        Tests/AP_Test.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        libraries/MeshNode/Messages.cpp
        libraries/SPI/SPI.cpp
        libraries/RingBuffer/RingBuffer.cpp
        libraries/ChildrenTree/ChildrenTree.cpp
        )

target_include_directories(ap_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
        ${CMAKE_CURRENT_LIST_DIR}/libraries/RingBuffer
        ${CMAKE_CURRENT_LIST_DIR}/libraries/ChildrenTree
        )

target_link_libraries(ap_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        )

pico_enable_stdio_uart(ap_background 0)
pico_enable_stdio_usb(ap_background 1)
        

pico_add_extra_outputs(ap_background)


add_executable(sta_root
        Tests/root_sta.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        libraries/MeshNode/Messages.cpp
        libraries/SPI/SPI.cpp
        libraries/RingBuffer/RingBuffer.cpp
        libraries/ChildrenTree/ChildrenTree.cpp
        )

target_include_directories(sta_root PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
        ${CMAKE_CURRENT_LIST_DIR}/libraries/RingBuffer
        ${CMAKE_CURRENT_LIST_DIR}/libraries/ChildrenTree
        )

target_link_libraries(sta_root
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        )

pico_enable_stdio_uart(sta_root 0)
pico_enable_stdio_usb(sta_root 1)
        

pico_add_extra_outputs(sta_root)

add_executable(ap_root
        Tests/ap_root.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        libraries/MeshNode/Messages.cpp
        libraries/SPI/SPI.cpp
        libraries/RingBuffer/RingBuffer.cpp
        libraries/ChildrenTree/ChildrenTree.cpp
        )

target_include_directories(ap_root PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
        ${CMAKE_CURRENT_LIST_DIR}/libraries/RingBuffer
        ${CMAKE_CURRENT_LIST_DIR}/libraries/ChildrenTree
        )

target_link_libraries(ap_root
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        )

pico_enable_stdio_uart(ap_root 0)
pico_enable_stdio_usb(ap_root 1)
        

pico_add_extra_outputs(ap_root)