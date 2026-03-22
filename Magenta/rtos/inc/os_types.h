#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <stdint.h> // fixed dimension type 
#include <stddef.h> // structure and memory 

/**
 * Defines the data type for a stack entry.
 * On a 32-bit architecture, this should be a 32-bit unsigned integer.
 */

typedef uint32_t os_stack_t;

/**
 * Defines the data type for a system tick count.
 */

typedef uint32_t os_tick_t;

/**
 * Defines the data type for task priority.
 * A lower number typically represents a higher priority.
 */

typedef uint8_t  os_priority_t;  // Range: 0-255

typedef enum {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED
} os_task_state_t;

/**
 * Task Control Block (TCB)
 * Tracks the state, stack, and metadata of each task.
 */
typedef struct os_tcb {
    os_stack_t      *stackPtr;      /* Current Stack Pointer */
    struct os_tcb   *next;          /* Circular ready list */
    struct os_tcb   *wait_next;     /* Synchronization wait list */
    
    /* Real-Time & Safety */
    os_priority_t    priority;      /* Lower = Higher priority */
    os_task_state_t  state;         /* Ready, Running, Blocked */
    
    /* MPU & Stack Guard */
    os_stack_t      *stack_base;    /* Start of stack RAM */
    uint32_t         stack_size;    /* Size in os_stack_t units */
    
    /* Metadata */
    uint32_t         task_id;
    uint32_t         sleep_ticks;
} os_tcb_t;

/**
 * Defines the standard status codes returned by OS functions.
 */
typedef enum {
    OS_OK = 0,              /* Operation was SUCCESSFUL */
    OS_ERR_PARAM,           /* An invalid parameter */
    OS_ERR_STACK_OVERFLOW,  /* A task stack has overflowed */
    OS_ERR_RESOURCE_BUSY    /* A requested resource is unavailable */
} os_status_t;

#endif