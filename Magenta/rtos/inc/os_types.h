#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <stdint.h> // portabilità garantita
#include <stddef.h>

typedef uint32_t os_stack_t;
typedef uint32_t os_tick_t;
typedef uint8_t  os_priority_t;  // 0-255

typedef enum {
    OS_OK = 0,
    OS_ERR_PARAM,
    OS_ERR_STACK_OVERFLOW,
    OS_ERR_RESOURCE_BUSY
} os_status_t;

#endif