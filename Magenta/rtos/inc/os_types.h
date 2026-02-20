#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <stdint.h> // Guaranteed portability
#include <stddef.h>

/**
 * @brief Defines the data type for a stack entry.
 * On a 32-bit architecture, this should be a 32-bit unsigned integer.
 */
typedef uint32_t os_stack_t;

/**
 * @brief Defines the data type for a system tick count.
 */
typedef uint32_t os_tick_t;

/**
 * @brief Defines the data type for task priority.
 * A lower number typically represents a higher priority.
 */
typedef uint8_t  os_priority_t;  // Range: 0-255

/**
 * @brief Defines the standard status codes returned by OS functions.
 */
typedef enum {
    OS_OK = 0,              /**< Operation was successful. */
    OS_ERR_PARAM,           /**< An invalid parameter was provided. */
    OS_ERR_STACK_OVERFLOW,  /**< A task stack has overflowed. */
    OS_ERR_RESOURCE_BUSY    /**< A requested resource is currently unavailable. */
} os_status_t;

#endif