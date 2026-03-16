cmake_minimum_required(VERSION 3.13)

# Imposta forzatamente la board a Pico 2 (RP2350)
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force the correct CPU, FPU, and ABI flags globally.
# This is a "bigger hammer" to ensure the linker gets the right settings,
# as target-specific properties were not working.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16" CACHE INTERNAL "C Flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16" CACHE INTERNAL "CXX Flags")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16" CACHE INTERNAL "ASM Flags")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=cortex-m33 -mthumb" CACHE INTERNAL "Linker Flags")


# 1. Import the Pico setup
include(pico_sdk_import.cmake)

# Project definition. ASM is vital for Layer 1 (os_port_asm.s)
project(Magenta C CXX ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 2. Initialize the SDK
pico_sdk_init()

# Define an option to choose which test application to build
option(BUILD_TEST_TYPE "Choose the test application to build (L1, L2)" "L1")

# Set the source file based on the option
set(TEST_APP_SOURCE "")
if (BUILD_TEST_TYPE STREQUAL "L1")
    set(TEST_APP_SOURCE "tests/main.c")
    message(STATUS "Building Layer 1 Test (main.c)")
elseif (BUILD_TEST_TYPE STREQUAL "L2")
    set(TEST_APP_SOURCE "tests/test_sync.c")
    message(STATUS "Building Layer 2 Test (test_sync.c)")
else()
    message(FATAL_ERROR "Invalid BUILD_TEST_TYPE specified: ${BUILD_TEST_TYPE}. Use L1 or L2.")
endif()

add_executable(Magenta
    ${TEST_APP_SOURCE}

    # Layer 1: Kernel Base
    rtos/src/os_kernel.c
    rtos/src/os_time.c
    rtos/port/os_port.c
    rtos/port/os_port_asm.s

    # Layer 2: IPC & Sync
    rtos/src/os_sync.c
)

# 3. Include Directories
target_include_directories(Magenta PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/rtos/inc
    ${CMAKE_CURRENT_LIST_DIR}/rtos/port
    ${CMAKE_CURRENT_LIST_DIR}/drivers
)

# 4. Link to Pico libraries
target_link_libraries(Magenta PRIVATE
    pico_stdlib
    hardware_gpio
    hardware_uart
    hardware_exception
)

# 5. Enable UART/USB output
pico_enable_stdio_usb(Magenta 1)
pico_enable_stdio_uart(Magenta 1)

# 6. Generate .uf2 file for flashing
pico_add_extra_outputs(Magenta)