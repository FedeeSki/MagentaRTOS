#include "os_syscall.h"
#include "os_kernel.h"
#include <stdio.h>
#include <stdarg.h>

/* Forward declarations of kernel internal functions */
extern void OS_Internal_Delay(uint32_t ticks);

/**
 * C Dispatcher for System Calls
 * @param id: The SVC number (passed in R0 from assembly)
 * @param args: Pointer to the stacked registers (passed in R1 from assembly)
 */
void OS_Syscall_Handler(uint32_t id, uint32_t *args) {
    /* args[0] = R0, args[1] = R1, args[2] = R2, args[3] = R3 
       When calling SVC #2 (PRINT), the original R0 (now args[0]) 
       contains the format string pointer. */
    
    switch(id) {
        case SYSCALL_YIELD:
            /* Trigger PendSV */
            *((volatile uint32_t *)0xE000ED04) = (1UL << 28);
            break;
            
        case SYSCALL_DELAY:
            /* args[0] was the 'ms' parameter passed via R0 */
            OS_Internal_Delay(args[0]);
            break;
            
        case SYSCALL_PRINT:
            /* Print directly using the string pointer in args[0] */
            if (args[0] != 0) {
                printf((const char*)args[0]);
            }
            break;
            
        default:
            /* Unknown Syscall */
            break;
    }
}

/* --- User Mode API Wrappers --- */

__attribute__((naked)) void OS_Yield(void) {
    __asm volatile (
        "svc #0 \n"
        "bx lr"
    );
}

__attribute__((naked)) void OS_Delay(uint32_t ms) {
    __asm volatile (
        "svc #1 \n"
        "bx lr"
    );
}

/**
 * Simplified SafePrintf for User Mode.
 * It passes the format string pointer to the kernel via SVC.
 */
void OS_SafePrintf(const char *format, ...) {
    /* In this basic version, we just pass the pointer to the format string.
       The kernel (privileged) will handle the actual printf. */
    __asm volatile (
        "mov r0, %0 \n"
        "svc #2"
        : : "r" (format) : "r0"
    );
}
