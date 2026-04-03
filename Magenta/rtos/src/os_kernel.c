#include "os_kernel.h"
#include "os_mpu.h"
#include <stddef.h>
#include <stdio.h>

/* Global kernel state */
os_tcb_t *currentTCB = NULL;
static uint32_t next_task_id = 0;

/* Idle task resources */
#define IDLE_STACK_SIZE 256
__attribute__((aligned(32))) static os_stack_t idle_stack[IDLE_STACK_SIZE];
static os_tcb_t idleTCB;

void OS_IdleTask(void) {
    while(1) {
        __asm("WFI");
    }
}

void OS_Init(void) {
    printf("[KERNEL] Initializing OS (Optimized TCB)...\n");
    currentTCB = NULL;
    next_task_id = 0;

    OS_TaskCreate(&idleTCB, OS_IdleTask, idle_stack, IDLE_STACK_SIZE, 0);
    OS_MPU_Init();
}

static os_tcb_t* OS_Get_Highest_Priority_Task(void) {
    if (currentTCB == NULL) return NULL;
    os_tcb_t *best_tcb = NULL;
    os_priority_t max_priority = 0;
    os_tcb_t *temp = currentTCB;
    do {
        if (temp->state == OS_TASK_STATE_READY) {
            if (temp->priority > max_priority) {
                max_priority = temp->priority;
                best_tcb = temp;
            }
        }
        temp = temp->next;
    } while (temp != currentTCB);
    return best_tcb ? best_tcb : &idleTCB;
}

void OS_Scheduler(void) {
    if (currentTCB == NULL) return;
    os_tcb_t *nextTCB = OS_Get_Highest_Priority_Task();
    if (nextTCB == currentTCB) {
        os_tcb_t *search = currentTCB->next;
        while (search != currentTCB) {
            if (search->state == OS_TASK_STATE_READY && search->priority == currentTCB->priority) {
                nextTCB = search;
                break;
            }
            search = search->next;
        }
    }
    currentTCB = nextTCB;
}

extern os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top);
extern void OS_Port_InitTick(uint32_t tick_ms);
extern void OS_Port_EnableFPU(void);

os_status_t OS_TaskCreate(os_tcb_t *tcb, void (*task_func)(void), os_stack_t *stack_base, uint32_t stack_size, os_priority_t priority) {
    if (tcb == NULL || task_func == NULL || stack_base == NULL || stack_size == 0) {
        return OS_ERR_PARAM;
    }
    uintptr_t top_address = (uintptr_t)stack_base + (stack_size * sizeof(os_stack_t));
    os_stack_t *stack_top = (os_stack_t *)(top_address & ~0x7);
    tcb->stackPtr = OS_Port_StackInit(task_func, stack_top);
    tcb->task_id = next_task_id++;
    tcb->stack_base = stack_base;
    tcb->stack_size = stack_size;
    tcb->state = OS_TASK_STATE_READY;
    tcb->priority = priority;
    tcb->base_priority = priority;
    tcb->sleep_ticks = 0;
    tcb->owned_mutexes = NULL;
    tcb->flags = OS_TASK_FLAG_PRIVILEGED;
    tcb->reserved = 0;
    
    printf("[KERNEL] Task %lu Created (Prio: %u, Stack: %p)\n", tcb->task_id, tcb->priority, tcb->stack_base);
    
    OS_ENTER_CRITICAL();
    if (currentTCB == NULL) {
        currentTCB = tcb;
        tcb->next = tcb;
    } else {
        tcb->next = currentTCB->next;
        currentTCB->next = tcb;
    }
    OS_EXIT_CRITICAL();
    return OS_OK;
}

void OS_Start(void) {
    printf("[KERNEL] Launching Scheduler...\n");
    OS_Port_EnableFPU();
    OS_Port_InitTick(1);
    
    OS_ENTER_CRITICAL();
    __asm volatile ("msr psp, %0" : : "r" (0));
    *((volatile uint32_t *)0xE000ED04) = (1UL << 28); // Trigger PendSV
    OS_EXIT_CRITICAL();
    
    while(1);
}
