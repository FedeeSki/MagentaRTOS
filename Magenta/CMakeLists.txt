cmake_minimum_required(VERSION 3.13)

# 1. Importa il setup del Pico
include(pico_sdk_import.cmake)

project(Magenta C CXX ASM) # ASM è vitale per il Layer 1 (os_port_asm.s)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 2. Inizializza l'SDK per RP2350
pico_sdk_init()

add_executable(Magenta
    tests/main.c
    # Layer 1: Kernel Base
    rtos/src/os_kernel.c
    rtos/src/os_time.c
    rtos/port/os_port.c
    rtos/port/os_port_asm.s
    # Layer 2 & 3 (saranno vuoti all'inizio, commentali se non esistono ancora)
    # rtos/src/os_sync.c
    # rtos/port/os_svc.s
)

# 3. Include Directories: permette di fare #include "os_kernel.h" da ovunque
target_include_directories(Magenta PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/rtos/inc
    ${CMAKE_CURRENT_LIST_DIR}/rtos/port
    ${CMAKE_CURRENT_LIST_DIR}/drivers
)

# 4. Link alle librerie del Pico (standard e hardware)
target_link_libraries(Magenta pico_stdlib hardware_exception)

# 5. Generazione file .uf2 per il flash
pico_add_extra_outputs(Magenta)