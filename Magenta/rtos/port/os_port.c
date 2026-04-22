#include "os_kernel.h"
#include "os_port.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include <stdio.h>

#define RED_LED_PIN 14

/* Helper per lampeggio diagnostico "Morse" (Presume gpio_init già chiamato) */
static void panic_blink(int flashes, int speed_ms) {
    while(1) {
        for(int i=0; i<flashes; i++) {
            gpio_put(RED_LED_PIN, 1);
            for(volatile int d=0; d<(speed_ms*10000); d++);
            gpio_put(RED_LED_PIN, 0);
            for(volatile int d=0; d<(speed_ms*10000); d++);
        }
        for(volatile int d=0; d<10000000; d++);
    }
}

/* Handle critical system faults */
void isr_hardfault(void) {
    OS_ENTER_CRITICAL();
    panic_blink(3, 10);
}

void isr_memmanage(void) {
    OS_ENTER_CRITICAL();
    panic_blink(1, 50);
}

/* Constants for stack frame initialization */
#define INITIAL_XPSR 0x01000000
#define INITIAL_LR   0xFFFFFFFD

/* Initialize task stack with a fake context */
os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top) {
    stack_top = (os_stack_t *)(((uintptr_t)stack_top) & ~0x7);

    /* Hardware Stack Frame */
    stack_top--; *stack_top = INITIAL_XPSR;
    stack_top--; *stack_top = (os_stack_t)task_func;
    stack_top--; *stack_top = 0xDEADBEEF; 
    stack_top--; *stack_top = 0x12121212; 
    stack_top--; *stack_top = 0x03030303; 
    stack_top--; *stack_top = 0x02020202; 
    stack_top--; *stack_top = 0x01010101; 
    stack_top--; *stack_top = 0x00000000; 

    /* Software Stack Frame */
    stack_top--; *stack_top = INITIAL_LR;
    stack_top--; *stack_top = 0x00000000; 
    stack_top--; *stack_top = 0x11111111; 
    stack_top--; *stack_top = 0x10101010; 
    stack_top--; *stack_top = 0x09090909; 
    stack_top--; *stack_top = 0x08080808; 
    stack_top--; *stack_top = 0x07070707; 
    stack_top--; *stack_top = 0x06060606; 
    stack_top--; *stack_top = 0x05050505; 
    stack_top--; *stack_top = 0x04040404; 

    return stack_top;
}

void OS_Port_InitTick(uint32_t tick_ms) {
    /* Set PendSV and SysTick to lowest priority (255) */
    /* SHPR3 (0xE000ED20) bits [23:16] = PendSV, bits [31:24] = SysTick */
    *((volatile uint32_t *)0xE000ED20) |= (0xFFUL << 16) | (0xFFUL << 24);
    
    uint32_t cpu_freq = clock_get_hz(clk_sys);
    uint32_t ticks = (cpu_freq / 1000) * tick_ms;
    *((volatile uint32_t *)0xE000E014) = ticks - 1; // RVR
    *((volatile uint32_t *)0xE000E010) = 0x7; // CSR (Enable, Int, Source)
}

extern void OS_Time_Update(void);

void isr_systick(void) {
    static uint32_t tick_count = 0;
    OS_Time_Update();
    *((volatile uint32_t *)0xE000ED04) = (1UL << 28); // Trigger PendSV

    if (++tick_count >= 1000) {
        tick_count = 0;
        printf("[SYS] Heartbeat\n");
    }
}

void OS_Port_EnableFPU(void) {
    *((volatile uint32_t *)0xE000ED88) |= (0xF << 20);
    __asm volatile ("dsb");
    __asm volatile ("isb");
}
