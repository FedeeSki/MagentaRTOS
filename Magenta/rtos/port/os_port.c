#include "os_kernel.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include <stdio.h>

/* Interrupt management helpers */
static inline void __disable_irq(void) {
    __asm volatile ("CPSID I" : : : "memory");
}

static inline void __enable_irq(void) {
    __asm volatile ("CPSIE I" : : : "memory");
}

/* ARM Cortex-M System Control Block and SysTick registers */
#define SCB_SHPR3           (*((volatile uint32_t *)0xE000ED20))
#define SCB_ICSR            (*((volatile uint32_t *)0xE000ED04))
#define SYSTICK_CSR         (*((volatile uint32_t *)0xE000E010))
#define SYSTICK_RVR         (*((volatile uint32_t *)0xE000E014))

/* Bit fields for the registers */
#define SCB_ICSR_PENDSVSET_BIT  (1UL << 28)
#define SYSTICK_CSR_ENABLE_BIT  (1UL << 0)
#define SYSTICK_CSR_TICKINT_BIT (1UL << 1)
#define SYSTICK_CSR_CLKSRC_BIT  (1UL << 2)

/* Handle critical system faults */
void isr_hard_fault(void) {
    __disable_irq();
    /* Wait for UART to finish current transmission */
    for(volatile int i=0; i<1000000; i++);
    printf("\n!!! HARD FAULT DETECTED !!!\n");
    while(1);
}

/* Constants for stack frame initialization */
#define INITIAL_XPSR 0x01000000
#define INITIAL_LR   0xFFFFFFFD

/* Initialize task stack with a fake context for the first switch */
os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top) {
    /* Align stack to 8 bytes */
    stack_top = (os_stack_t *)(((uintptr_t)stack_top) & ~0x7);

    /* Hardware Stack Frame (restored by CPU) */
    stack_top--; *stack_top = INITIAL_XPSR;
    stack_top--; *stack_top = (os_stack_t)task_func;
    stack_top--; *stack_top = 0xDEADBEEF; /* Initial task LR */
    stack_top--; *stack_top = 0x12121212; /* R12 */
    stack_top--; *stack_top = 0x03030303; /* R3 */
    stack_top--; *stack_top = 0x02020202; /* R2 */
    stack_top--; *stack_top = 0x01010101; /* R1 */
    stack_top--; *stack_top = 0x00000000; /* R0 */

    /* Software Stack Frame (restored by PendSV) */
    /* 10 registers for 8-byte alignment: {R4-R11, R12, LR} */
    stack_top--; *stack_top = INITIAL_LR;
    stack_top--; *stack_top = 0x00000000; /* R12 padding */
    stack_top--; *stack_top = 0x11111111; /* R11 */
    stack_top--; *stack_top = 0x10101010; /* R10 */
    stack_top--; *stack_top = 0x09090909; /* R9 */
    stack_top--; *stack_top = 0x08080808; /* R8 */
    stack_top--; *stack_top = 0x07070707; /* R7 */
    stack_top--; *stack_top = 0x06060606; /* R6 */
    stack_top--; *stack_top = 0x05050505; /* R5 */
    stack_top--; *stack_top = 0x04040404; /* R4 */

    return stack_top;
}

/* Set up SysTick timer for the OS heartbeat */
void OS_Port_InitTick(uint32_t tick_ms) {
    /* Set PendSV to lowest priority */
    SCB_SHPR3 |= (0xFF << 16);

    uint32_t cpu_freq = clock_get_hz(clk_sys);
    uint32_t ticks = (cpu_freq / 1000) * tick_ms;

    SYSTICK_RVR = ticks - 1;
    SYSTICK_CSR = (SYSTICK_CSR_CLKSRC_BIT | SYSTICK_CSR_TICKINT_BIT | SYSTICK_CSR_ENABLE_BIT);
}

extern void OS_Time_Update(void);

/* SysTick handler updates time and triggers context switch */
void isr_systick(void) {
    static uint32_t tick_count = 0;
    OS_Time_Update();
    SCB_ICSR = SCB_ICSR_PENDSVSET_BIT;

    /* Heartbeat log every 1000 ms */
    if (++tick_count >= 1000) {
        tick_count = 0;
        printf("[SYS] Heartbeat\n");
    }
}

/* Enable FPU for Cortex-M33 */
void OS_Port_EnableFPU(void) {
    #define CPACR (*((volatile uint32_t *)0xE000ED88))
    CPACR |= (0xF << 20);

    __asm volatile ("dsb");
    __asm volatile ("isb");
}

extern os_tcb_t *currentTCB;
