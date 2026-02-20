#include "os_kernel.h"
#include <stddef.h>
#include <stdio.h>

/* Interrupt management helpers */
static inline void __disable_irq(void) {
    __asm volatile ("CPSID I" : : : "memory");
}

static inline void __enable_irq(void) {
    __asm volatile ("CPSIE I" : : : "memory");
}

/* Global kernel state */
os_tcb_t *currentTCB = NULL;
static uint32_t next_task_id = 0;

/* Idle task resources */
#define IDLE_STACK_SIZE 256
static os_stack_t idle_stack[IDLE_STACK_SIZE];
static os_tcb_t idleTCB;

/* The idle task runs when no other task is ready */
void OS_IdleTask(void) {
    while(1) {
        /* Wait for interrupt to save power */
        __asm("WFI");
    }
}

/* Initialize kernel and create the idle task */
void OS_Init(void) {
    printf("[KERNEL] Initializing OS...\n");
    currentTCB = NULL;
    next_task_id = 0;

    OS_TaskCreate(&idleTCB, OS_IdleTask, idle_stack, IDLE_STACK_SIZE);
    printf("[KERNEL] Idle Task created.\n");
}

/* Simple round-robin scheduler */
void OS_Scheduler(void) {
    if (currentTCB == NULL) return;

    os_tcb_t *nextTCB = currentTCB->next;

    /* Find next ready task */
    while (nextTCB->state != TASK_STATE_READY) {
        nextTCB = nextTCB->next;
    }

    currentTCB = nextTCB;
}

/* Porting functions defined in os_port.c */
extern os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top);
extern void OS_Port_InitTick(uint32_t tick_ms);
extern void OS_Port_EnableFPU(void);

/* Create a new task and add it to the circular list */
os_status_t OS_TaskCreate(os_tcb_t *tcb, void (*task_func)(void), os_stack_t *stack_base, uint32_t stack_size) {
    if (tcb == NULL || task_func == NULL || stack_base == NULL || stack_size == 0) {
        return OS_ERR_PARAM;
    }

    /* Align stack to 8 bytes */
    uintptr_t top_address = (uintptr_t)stack_base + (stack_size * sizeof(os_stack_t));
    os_stack_t *stack_top = (os_stack_t *)(top_address & ~0x7);

    /* Initialize stack frame */
    tcb->stackPtr = OS_Port_StackInit(task_func, stack_top);
    tcb->state = TASK_STATE_READY;
    tcb->sleep_ticks = 0;
    tcb->task_id = next_task_id++;

    printf("[KERNEL] Created Task %lu (Func: %p, SP: %p)\n", tcb->task_id, task_func, tcb->stackPtr);

    /* Insert into circular list */
    __disable_irq();
    if (currentTCB == NULL) {
        currentTCB = tcb;
        tcb->next = tcb;
    } else {
        tcb->next = currentTCB->next;
        currentTCB->next = tcb;
    }
    __enable_irq();

    return OS_OK;
}

/* Start the OS and switch to the first task */
void OS_Start(void) {
    printf("[KERNEL] OS_Start: Configuring hardware and launching first task...\n");
    
    /* Enable FPU */
    OS_Port_EnableFPU();

    /* Prepare hardware tick */
    OS_Port_InitTick(1);

    /* Final setup with interrupts disabled */
    __disable_irq();

    /* Set PSP to 0 signals PendSV to skip save */
    __asm volatile ("msr psp, %0" : : "r" (0));

    /* Trigger PendSV */
    *((volatile uint32_t *)0xE000ED04) = (1UL << 28);

    /* Enable interrupts to trigger the switch */
    __enable_irq();
    
    /* We should never reach this line as the stack pointer changes to Task 1 */
    while(1);
}
