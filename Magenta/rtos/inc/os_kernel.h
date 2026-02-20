#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include "os_types.h"

/* Possible task states */
typedef enum {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED
} os_task_state_t;

/* Task Control Block (TCB) */
typedef struct os_tcb {
    os_stack_t      *stackPtr;
    struct os_tcb   *next;
    os_priority_t    priority;
    os_task_state_t  state;
    uint32_t         task_id;
    uint32_t         sleep_ticks;
} os_tcb_t;

/* Kernel API */
void OS_Init(void);
void OS_Start(void);
os_status_t OS_TaskCreate(os_tcb_t *tcb, void (*task_func)(void), os_stack_t *stack_base, uint32_t stack_size);

/* Scheduler called by PendSV */
void OS_Scheduler(void);

/* Time management */
void OS_Delay(uint32_t ticks);
void OS_Time_Update(void);

#endif
