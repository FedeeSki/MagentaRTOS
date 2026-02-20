# MagentaRTOS

Preemptive Real-Time Operating System (RTOS) engineered for the **Raspberry Pi Pico 2 (RP2350)** with native ARM Cortex-M33 support.

<img src="https://docs.sunfounder.com/projects/pico-2w-kit/en/latest/_images/pin_pic3.png" alt="Pi Pico 2" width="50%"/>


## Roadmap

![Layer 1](https://img.shields.io/badge/Layer_1%3A_Kernel-READY-brightgreen?style=for-the-badge)  

![Layer 2](https://img.shields.io/badge/Layer_2%3A_IPC_%26_Synchronization-IN_PROGRESS-magenta?style=for-the-badge)  

![Layer 3](https://img.shields.io/badge/Layer_3%3A_Protection_%26_Safety-PLANNED-lightgrey?style=for-the-badge)  

![Layer 4](https://img.shields.io/badge/Layer_4%3A_Power_Management_%26_Drivers-PLANNED-lightgrey?style=for-the-badge)  

 

## Contents
1. [Overview](#overview)
2. [Milestones: Layer 1](#milestone-layer-1-kernel-core)
3. [Observational Notes](#observational-notes)
4. [Development Lifecycle](#development-lifecycle)
5. [Prerequisites](#prerequisites)
6. [Building & Flashing](#building-flashing)

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

   ```bash
   mkdir build && cd build 
   cmake ..
   make -j$(nproc)
   ```

Connect the Pico 2 in **BOOTSEL** mode and copy the generated .uf2 file: 

```bash
cp Magenta.uf2 /path/to/RPI-RP2/
```
