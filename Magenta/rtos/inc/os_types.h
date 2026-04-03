#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t os_stack_t;
typedef uint32_t os_tick_t;
typedef uint8_t  os_priority_t;

/* Task States (uint8_t) */
#define OS_TASK_STATE_READY     0
#define OS_TASK_STATE_RUNNING   1
#define OS_TASK_STATE_BLOCKED   2
#define OS_TASK_STATE_SLEEPING  3

/* Task Flags (uint8_t) */
#define OS_TASK_FLAG_NONE        0x00
#define OS_TASK_FLAG_PRIVILEGED  (1 << 0)
#define OS_TASK_FLAG_FPU_ACTIVE  (1 << 1)
#define OS_TASK_FLAG_STATIC      (1 << 2)

/* Forward declarations */
struct os_mutex;

/**
 * Task Control Block (TCB) - Optimized Layout (40 bytes)
 * 32-bit aligned fields first, then grouped control bytes.
 */
typedef struct os_tcb {
    os_stack_t      *stackPtr;      /* Offset 0  */
    struct os_tcb   *next;          /* Offset 4  */
    struct os_tcb   *wait_next;     /* Offset 8  */
    struct os_mutex *owned_mutexes; /* Offset 12 */
    
    os_stack_t      *stack_base;    /* Offset 16 */
    uint32_t         stack_size;    /* Offset 20 */
    
    uint32_t         task_id;       /* Offset 24 */
    uint32_t         sleep_ticks;   /* Offset 28 */
    
    uint8_t          state;         /* Offset 32 */
    os_priority_t    priority;      /* Offset 33 */
    os_priority_t    base_priority; /* Offset 34 */
    uint8_t          flags;         /* Offset 35 */
    
    uint32_t         reserved;      /* Offset 36 - Padding to 40 bytes */
} os_tcb_t;

typedef enum {
    OS_OK = 0,
    OS_ERR_PARAM,
    OS_ERR_STACK_OVERFLOW,
    OS_ERR_RESOURCE_BUSY
} os_status_t;

#endif /* OS_TYPES_H */
