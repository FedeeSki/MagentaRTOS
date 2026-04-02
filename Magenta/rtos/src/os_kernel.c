#include "os_kernel.h"
#include <stddef.h>
#include <stdio.h>

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
        __asm("WFI");
    }
}

/* Initialize kernel and create the idle task with lowest priority (0) */
void OS_Init(void) {
    printf("[KERNEL] Initializing OS (Priority-based Scheduler)...\n");
    currentTCB = NULL;
    next_task_id = 0;

    /* Priority 0 is the lowest in our increasing priority policy */
    OS_TaskCreate(&idleTCB, OS_IdleTask, idle_stack, IDLE_STACK_SIZE, 0);
}

/**
 * Finds the highest priority task that is in the READY state.
 * Implementation: Linear scan (O(n)) prepared for Bitmask optimization.
 */
static os_tcb_t* OS_Get_Highest_Priority_Task(void) {
    if (currentTCB == NULL) return NULL;

    os_tcb_t *best_tcb = NULL;
    os_priority_t max_priority = 0;
    os_tcb_t *temp = currentTCB;

    /* Traverse the circular list to find the highest priority READY task */
    do {
        if (temp->state == TASK_STATE_READY) {
            if (temp->priority > max_priority) {
                max_priority = temp->priority;
                best_tcb = temp;
            } else if (temp->priority == max_priority && best_tcb != NULL) {
                /* If priorities are equal, follow circular order (Round-Robin) */
            }
        }
        temp = temp->next;
    } while (temp != currentTCB);

    /* If no other task is ready, fallback to Idle (which is priority 0 and always ready) */
    return best_tcb ? best_tcb : &idleTCB;
}

/* Priority-based Scheduler */
void OS_Scheduler(void) {
    if (currentTCB == NULL) return;

    os_tcb_t *nextTCB = OS_Get_Highest_Priority_Task();

    /* 
     * If the highest priority task found is the same as current, 
     * but there are other tasks with the SAME priority, 
     * we should perform a Round-Robin step to be fair.
     */
    if (nextTCB == currentTCB) {
        os_tcb_t *search = currentTCB->next;
        while (search != currentTCB) {
            if (search->state == TASK_STATE_READY && search->priority == currentTCB->priority) {
                nextTCB = search;
                break;
            }
            search = search->next;
        }
    }

    currentTCB = nextTCB;
}

/* Porting functions defined in os_port.c */
extern os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top);
extern void OS_Port_InitTick(uint32_t tick_ms);
extern void OS_Port_EnableFPU(void);

/* Create a new task with specified priority */
os_status_t OS_TaskCreate(os_tcb_t *tcb, void (*task_func)(void), os_stack_t *stack_base, uint32_t stack_size, os_priority_t priority) {
    if (tcb == NULL || task_func == NULL || stack_base == NULL || stack_size == 0) {
        return OS_ERR_PARAM;
    }
    
    uintptr_t top_address = (uintptr_t)stack_base + (stack_size * sizeof(os_stack_t));
    os_stack_t *stack_top = (os_stack_t *)(top_address & ~0x7);

    /* Initialize TCB fields */
    tcb->stackPtr = OS_Port_StackInit(task_func, stack_top);
    tcb->state = TASK_STATE_READY;
    tcb->sleep_ticks = 0;
    tcb->task_id = next_task_id++;
    
    /* MPU & Priority */
    tcb->stack_base = stack_base;
    tcb->stack_size = stack_size;
    tcb->priority = priority;
    tcb->base_priority = priority;
    tcb->owned_mutex_list = NULL;

    printf("[KERNEL] Created Task %lu (Prio: %u, Func: %p)\n", tcb->task_id, tcb->priority, task_func);

    /* Insert into circular list */
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

/* Start the OS and switch to the first task */
void OS_Start(void) {
    printf("[KERNEL] OS_Start: Launching Scheduler (Increasing Priority Policy)...\n");
    
    OS_Port_EnableFPU();
    OS_Port_InitTick(1);

    OS_ENTER_CRITICAL();
    __asm volatile ("msr psp, %0" : : "r" (0));
    *((volatile uint32_t *)0xE000ED04) = (1UL << 28);
    OS_EXIT_CRITICAL();
    
    while(1);
}
