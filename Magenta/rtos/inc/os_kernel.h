#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include "os_types.h"
#include "os_port.h"

/* Kernel API */
void OS_Init(void);
void OS_Start(void);
os_status_t OS_TaskCreate(os_tcb_t *tcb, void (*task_func)(void), os_stack_t *stack_base, uint32_t stack_size, os_priority_t priority);

/* Static Allocation Macros */
#define OS_TASK_STACK_DEFINE(name, size) \
    __attribute__((aligned(32))) static os_stack_t name##_stack[size]

#define OS_TASK_TCB_DEFINE(name) \
    __attribute__((aligned(4))) static os_tcb_t name##_tcb

#define OS_TASK_CREATE_STATIC(name, task_func, stack_size, priority) \
    OS_TaskCreate(&name##_tcb, task_func, name##_stack, stack_size, priority)

/* Scheduler called by PendSV */
void OS_Scheduler(void);

#endif
