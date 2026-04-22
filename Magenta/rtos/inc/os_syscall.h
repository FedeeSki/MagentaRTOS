#ifndef OS_SYSCALL_H
#define OS_SYSCALL_H

#include <stdint.h>

/* System Call IDs */
#define SYSCALL_YIELD   0
#define SYSCALL_DELAY   1
#define SYSCALL_PRINT   2

/* User-mode API wrappers */
void OS_Yield(void);
void OS_Delay(uint32_t ms);
void OS_SafePrintf(const char *format, ...);

#endif /* OS_SYSCALL_H */
