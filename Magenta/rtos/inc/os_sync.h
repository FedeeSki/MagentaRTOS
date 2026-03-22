#ifndef OS_SYNC_H
#define OS_SYNC_H

#include "os_kernel.h"

/**
 * Semaphore structure
 */
typedef struct {
    uint32_t    count;
    uint32_t    max_count;
    os_tcb_t   *wait_list;
} os_sem_t;

/**
 * Mutex structure
 */
typedef struct {
    os_tcb_t   *owner;
    uint32_t    lock_count;
    os_tcb_t   *wait_list;
} os_mutex_t;

/* Semaphore API */
void OS_SemInit(os_sem_t *sem, uint32_t initial_count, uint32_t max_count);
os_status_t OS_SemWait(os_sem_t *sem);
void OS_SemPost(os_sem_t *sem);

/* Mutex API */
void OS_MutexInit(os_mutex_t *mutex);
os_status_t OS_MutexLock(os_mutex_t *mutex);
void OS_MutexUnlock(os_mutex_t *mutex);

#endif
