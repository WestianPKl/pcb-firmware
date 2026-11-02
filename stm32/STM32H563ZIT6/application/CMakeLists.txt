# Set the minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.20)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

set(CMSIS ${CMAKE_CURRENT_SOURCE_DIR}/../cmsis)

# Set target definition variables required by the toolchain file
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)

# Set the toolchain file for cross-compilation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)

# Project definition
project(application)

# Enable CMake support for ASM and C/CPP languages
enable_language(C ASM)

set(PROJECT_NAME application)

# Create an executable object type
add_executable(${PROJECT_NAME})

# Include additional CMake files for flags, components, and files
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/components.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/files.cmake)

# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/input.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/output.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/timer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/support.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/uart.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/protocol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/i2c.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/spi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/adc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/ld2450.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/tca9548.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/sht40.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/mcp23017.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/bme280.c
  ${CMAKE_CURRENT_SOURCE_DIR}/AI/App/user_init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/AI/Network/network.c
  ${CMAKE_CURRENT_SOURCE_DIR}/AI/Network/network_data.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/ai.c
  # ${CMAKE_CURRENT_SOURCE_DIR}/Src/nrf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/can.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/ethernet.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/ethernetif.c
  # ${CMAKE_CURRENT_SOURCE_DIR}/Src/tcp_echo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/udp_echo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/systick.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/rtc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/card_manager.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/fmac.c
  ${CMSIS}/Device/ST/STM32H5xx/Source/Templates/system_stm32h5xx.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/def.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/inet_chksum.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/init.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/ip.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/mem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/memp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/netif.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/pbuf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/raw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/stats.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/sys.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/tcp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/tcp_in.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/tcp_out.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/timeouts.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/udp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/ipv4/etharp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/ipv4/icmp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/ipv4/ip4.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/ipv4/ip4_addr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/core/ipv4/ip4_frag.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/netif/ethernet.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/controller_tcp.c
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMSIS}/Device/ST/STM32H5xx/Include
  ${CMSIS}/Include
  ${CMAKE_CURRENT_SOURCE_DIR}/AI/Middlewares/ST/AI/Inc
  ${CMAKE_CURRENT_SOURCE_DIR}/AI/Network
  ${CMAKE_CURRENT_SOURCE_DIR}/AI/App
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lwIP/system
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/AI/Middlewares/ST/AI/Lib/NetworkRuntime1201_CM33_GCC.a
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
  # User defined compiler options
)

# Add additional defined symbols
target_compile_definitions(${PROJECT_NAME} PRIVATE
  STM32H563xx
)

# Linker options
target_link_options(${PROJECT_NAME} PRIVATE 
  # User defined linker options
)

# Add additional linker search directories
target_link_directories(${PROJECT_NAME} PRIVATE
  # Add additional linker search directories here
)

# Add additional linker libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Add additional linker libraries here
)
