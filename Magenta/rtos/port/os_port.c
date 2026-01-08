#include "os_kernel.h"

/* Costante xPSR: bit 24 (T) a 1 per indicare istruzioni Thumb (obbligatorio) */
#define INITIAL_XPSR 0x01000000

os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top) {
    /* Allineamento a 8 byte obbligatorio per ABI ARM */
    stack_top = (os_stack_t *)(((uintptr_t)stack_top) & ~0x7);

    /* --- HW FRAME (salvati/caricati dalla CPU) --- */
    stack_top--; *stack_top = INITIAL_XPSR;          /* xPSR */
    stack_top--; *stack_top = (os_stack_t)task_func;   /* PC (Program Counter) */
    stack_top--; *stack_top = 0xFFFFFFFD;            /* LR (Ritorno a Thread Mode, usa PSP) */
    stack_top--; *stack_top = 0x12121212;            /* R12 */
    stack_top--; *stack_top = 0x03030303;            /* R3 */
    stack_top--; *stack_top = 0x02020202;            /* R2 */
    stack_top--; *stack_top = 0x01010101;            /* R1 */
    stack_top--; *stack_top = 0x00000000;            /* R0 */

    /* --- SF FRAME (salvati/caricati dal PendSV_Handler) --- */
    stack_top--; *stack_top = 0x11111111;            /* R11 */
    stack_top--; *stack_top = 0x10101010;            /* R10 */
    stack_top--; *stack_top = 0x09090909;            /* R9 */
    stack_top--; *stack_top = 0x08080808;            /* R8 */
    stack_top--; *stack_top = 0x07070707;            /* R7 */
    stack_top--; *stack_top = 0x06060606;            /* R6 */
    stack_top--; *stack_top = 0x05050505;            /* R5 */
    stack_top--; *stack_top = 0x04040404;            /* R4 */

    return stack_top; /* Questo valore andrà in tcb->stackPtr */
}


/* Indirizzi dei registri di sistema ARM Cortex-M */
#define SYS_TICK_CTRL       (*((volatile uint32_t *)0xE000E010))
#define SYS_TICK_LOAD       (*((volatile uint32_t *)0xE000E014))
#define INT_CTRL_STATE      (*((volatile uint32_t *)0xE000ED04))
#define SYSPRI_3            (*((volatile uint32_t *)0xE000ED20))

/* Bit specifici */
#define PENDSVSET           (1 << 28)
#define TICK_ENABLE         (1 << 0)
#define TICK_INT            (1 << 1)
#define TICK_CLK_SRC        (1 << 2)

void OS_Port_InitTick(uint32_t tick_ms) {
    /* 1. Imposta la priorità di PendSV al minimo (0xFF)
       Fondamentale: PendSV deve scattare DOPO ogni altro interrupt hardware. */
    SYSPRI_3 |= (0xFF << 16);

    /* 2. Calcola i cicli di clock per il tick richiesto.
       L'RP2350 solitamente corre a 150MHz (150.000.000 cicli/secondo). */
    uint32_t cpu_freq = 150000000; 
    uint32_t ticks = (cpu_freq / 1000) * tick_ms;

    /* 3. Carica il valore nel timer e abilitalo */
    SYS_TICK_LOAD = ticks - 1;
    SYS_TICK_CTRL = (TICK_ENABLE | TICK_INT | TICK_CLK_SRC);
}

/* Dichiarazione della funzione definita in os_time.c */
extern void OS_Time_Update(void);

void SysTick_Handler(void) {
    /* 1. Aggiorna i delay dei task BLOCKED */
    OS_Time_Update();

    /* 2. Prenota il cambio contesto nel PendSV */
    INT_CTRL_STATE = PENDSVSET;
}