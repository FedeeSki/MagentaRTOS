#ifndef OS_PORT_H
#define OS_PORT_H

#include "os_types.h"

/* ARM Cortex-M ICSR Register address */
#define SCB_ICSR            (*((volatile uint32_t *)0xE000ED04))
#define SCB_ICSR_PENDSVSET_BIT  (1UL << 28)

/* Interrupt management macros */
#define OS_ENTER_CRITICAL()   __asm volatile ("CPSID I" : : : "memory")
#define OS_EXIT_CRITICAL()    __asm volatile ("CPSIE I" : : : "memory")

#define OS_TRIGGER_PENDSV()   (SCB_ICSR = SCB_ICSR_PENDSVSET_BIT)

/* Porting functions */
os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top);
void OS_Port_InitTick(uint32_t tick_ms);
void OS_Port_EnableFPU(void);

#endif
