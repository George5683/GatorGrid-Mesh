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

# Cmake for STA_testing
add_executable(sta_background
        Tests/STA_Test.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        libraries/SPI/SPI.cpp
        )

target_include_directories(sta_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
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
        libraries/SPI/SPI.cpp
        )

target_include_directories(ap_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
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

# Cmake for SPI MASTER
add_executable(SPI_MASTER
        Tests/SPI_MASTER_TEST.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        libraries/SPI/SPI.cpp
        )

target_include_directories(SPI_MASTER PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
        )

target_link_libraries(SPI_MASTER
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        )

pico_enable_stdio_uart(SPI_MASTER 0)
pico_enable_stdio_usb(SPI_MASTER 1)
        

pico_add_extra_outputs(SPI_MASTER)

# Cmake for SPI SLAVE
add_executable(SPI_SLAVE
        Tests/SPI_SLAVE_TEST.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        libraries/SPI/SPI.cpp
        )

target_include_directories(SPI_SLAVE PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/SPI
        ${CMAKE_CURRENT_LIST_DIR}/libraries/MeshNode
        )

target_link_libraries(SPI_SLAVE
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_spi
        )

pico_enable_stdio_uart(SPI_SLAVE 0)
pico_enable_stdio_usb(SPI_SLAVE 1)
        

pico_add_extra_outputs(SPI_SLAVE)

# Cmake for AccessPoint Example

add_executable(picow_access_point_background
        src/main.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        )

target_include_directories(picow_access_point_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        )

target_link_libraries(picow_access_point_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )

# You can change the address below to change the address of the access point
pico_configure_ip4_address(picow_access_point_background PRIVATE
        CYW43_DEFAULT_IP_AP_ADDRESS 192.168.4.1
        )
pico_add_extra_outputs(picow_access_point_background)

add_executable(picow_access_point_poll
        src/main.cpp
        libraries/dhcpserver/dhcpserver.c
        libraries/dnsserver/dnsserver.c
        libraries/MeshNode/MeshNodeAP.cpp
        libraries/MeshNode/MeshNodeSTA.cpp
        )

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(picow_access_point_poll 1)
pico_enable_stdio_usb(picow_access_point_poll 0)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(picow_access_point_background 0)
pico_enable_stdio_usb(picow_access_point_background 1)

target_include_directories(picow_access_point_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/libraries/dnsserver
        )
target_link_libraries(picow_access_point_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        )
# You can change the address below to change the address of the access point
pico_configure_ip4_address(picow_access_point_poll PRIVATE
        CYW43_DEFAULT_IP_AP_ADDRESS 192.168.4.1
        )
pico_add_extra_outputs(picow_access_point_poll)
