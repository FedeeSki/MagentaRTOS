# MagentaRTOS

Preemptive Real-Time Operating System (RTOS) engineered for the **Raspberry Pi Pico 2 (RP2350)** with native ARM Cortex-M33 support.

<img src="https://docs.sunfounder.com/projects/pico-2w-kit/en/latest/_images/pin_pic3.png" alt="Pi Pico 2" width="50%"/>


## Roadmap

![Layer 1](https://img.shields.io/badge/Layer_1%3A_Kernel-TESTING-brightgreen?style=for-the-badge)  

![Layer 2](https://img.shields.io/badge/Layer_2%3A_IPC_%26_Synchronization-WORKING-brightgreen?style=for-the-badge)  

![Layer 3](https://img.shields.io/badge/Layer_3%3A_Protection_%26_Safety-PLANNED-lightgrey?style=for-the-badge)  

![Layer 4](https://img.shields.io/badge/Layer_4%3A_Power_Management_%26_Drivers-PLANNED-lightgrey?style=for-the-badge)  

 

## Contents
1. [Overview](#overview)
2. [Milestones: Layer 1](#milestone-layer-1-kernel-core)
3. [Milestones: Layer 2](#milestones-layer-2-ipc--synchronization)
4. [Observational Notes](#observational-notes)
5. [Development Lifecycle](#development-lifecycle)
6. [Prerequisites](#prerequisites)
7. [Building & Flashing](#building-flashing)
8. [Serial Monitoring](#serial-monitoring)

---

## Overview
MagentaRTOS provides a deterministic execution environment utilizing a preemptive Round-Robin scheduler. It is specifically ported for the RP2350 architecture, leveraging the ARM Cortex-M33 feature set, including Lazy FPU Stacking and efficient context switching.

## Milestone: Layer 1 (Kernel Core)

### Architecture & Scheduling
* **Preemptive Round-Robin**: Deterministic task switching based on a circular TCB link list.
* **Time-Slicing**: 1ms hardware-timed quantum utilizing the ARM SysTick timer.
* **Task Lifecycle**: Full implementation of `READY`, `BLOCKED`, and `RUNNING` states.
* **Idle Management**: Mandatory background task with `WFI` (Wait-For-Interrupt) for optimized power consumption.

### Hardware Porting (ARM Cortex-M33 / RP2350)
* **Context Switching**: Low-latency switching implemented via the `PendSV` exception handler.
* **Lazy FPU Stacking**: Hardware-detected FPU usage; registers are saved/restored only when active, minimizing overhead.
* **EABI Alignment**: Guaranteed 8-byte stack alignment (10-register software frame) to prevent alignment faults.
* **Vector Table Integration**: Native compatibility with Pico SDK ISR naming convention (`isr_systick`, `isr_pendsv`).

### Time & Memory
* **Sleep Queue**: Millisecond-resolution delay resolution via `OS_Delay` API.
* **Static Allocation**: Task stacks and TCBs are statically allocated for deterministic memory footprints and safety.

## Milestones: Layer 2 (IPC & Synchronization)

### Primitives
*   **Semaphores**: Binary and Counting Semaphores for task synchronization and resource management.
*   **Mutexes**: Recursive Mutexes with ownership tracking to protect shared resources and prevent priority inversion (basic implementation).
*   **Wait Lists**: FIFO (First-In, First-Out) wait queues for tasks blocked on semaphores or mutexes.

### Kernel Enhancements
*   **Critical Sections**: Standardized `OS_ENTER_CRITICAL()` and `OS_EXIT_CRITICAL()` macros for atomic operations.
*   **Task State Management**: Refined `BLOCKED` state handling; tasks only unblocked by time if `sleep_ticks > 0`.
*   **Scheduler Integration**: Immediate context switching via `PendSV` upon task blocking/unblocking for efficient resource contention.

---

## Observational Notes

### Execution Jitter & Phase Drift
During multi-tasking stress tests with synchronized 250ms delays, a phase shift (drift) was observed between indicators.
* **Cause**: Cumulative execution overhead from blocking operations (e.g., UART `printf`).
* **Mechanism**: High-cycle operations executed *before* the `OS_Delay` call extend the real-world period to $Delay + ExecutionTime$.
* **Significance**: Confirms the scheduler's ability to manage asymmetric workloads without impacting global system stability.

### Acoustic Feedback (Capacitor Squeal)
The hardware emits a characteristic high-pitched sound (~1kHz) during the idle cycle.
* **Cause**: Piezoelectric effect in ceramic (MLCC) capacitors on the Pico 2 board.
* **Mechanism**: Rapid transients in current consumption as the CPU transitions from `WFI` (Idle) to `Active` state at a fixed 1000Hz frequency.
* **Significance**: Provides physical confirmation of a stable and precise 1ms System Tick.

---

## Prerequisites
* **Toolchain**: `arm-none-eabi-gcc` (ARM GCC Compiler).
* **Build System**: `CMake` (v3.13+).
* **Pico SDK**: Properly configured environment with the `PICO_SDK_PATH` variable.

## Building & Flashing

To build the project, follow these steps. You can choose which test application to build:

1.  **Navigate to the project root and create a build directory**:
    ```bash
    cd /Users/fedeeee/Desktop/MagentaRTOS
    mkdir -p build && cd build
    ```

2.  **Configure CMake to select the test layer**:
    *   **To build Layer 1 (Kernel Core) test (`main.c`):**
        ```bash
        cmake -DBUILD_TEST_TYPE=L1 ..
        ```
    *   **To build Layer 2 (IPC & Synchronization) test (`test_sync.c`):**
        ```bash
        cmake -DBUILD_TEST_TYPE=L2 ..
        ```
    (By default, `L1` is selected if `BUILD_TEST_TYPE` is not specified.)

3.  **Compile the project**:
    ```bash
    cmake --build .
    # or
    # make -j$(nproc)
    ```

Connect the Pico 2 in **BOOTSEL** mode and copy the generated `.uf2` file (e.g., `build/Magenta.uf2`) to the mounted RPI-RP2 drive:

```bash
cp Magenta.uf2 /path/to/RPI-RP2/
```

---

## Serial Monitoring

Per monitorare l'output seriale del Pico dal terminale, puoi usare `picocom`.

1.  **Installazione (se non lo hai già):**
    Su macOS, puoi installarlo tramite Homebrew:
    ```bash
    brew install picocom
    ```

2.  **Connessione alla Porta Seriale del Pico:**
    Una volta installato, puoi connetterti alla porta seriale del tuo Pico specificando la porta e il baud rate (generalmente 115200 per il Pico):

    ```bash
    picocom -b 115200 /dev/cu.usbmodem101
    ```
    Sostituisci `/dev/cu.usbmodem101` con la porta corretta, se fosse diversa (puoi trovarla con `ls /dev/cu.*`).

3.  **Funzionalità Utili di `picocom`:**
    *   **Uscire**: Per uscire da `picocom`, premi `Ctrl+A` seguito da `Ctrl+Q`.
    *   **Menu Aiuto**: Per vedere i comandi disponibili, premi `Ctrl+A` seguito da `Ctrl+H`.

`picocom` ti darà un'esperienza affidabile per il monitoraggio seriale.
