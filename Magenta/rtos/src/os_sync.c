#include "os_sync.h"
#include <stddef.h>

extern os_tcb_t *currentTCB;

/* Internal: Add a task to a wait list (FIFO) */
static void wait_list_add(os_tcb_t **list, os_tcb_t *tcb) {
    if (*list == NULL) {
        *list = tcb;
        tcb->wait_next = NULL;
    } else {
        os_tcb_t *temp = *list;
        while (temp->wait_next != NULL) {
            temp = temp->wait_next;
        }
        temp->wait_next = tcb;
        tcb->wait_next = NULL;
    }
}

/* Internal: Remove the first task from a wait list */
static os_tcb_t* wait_list_remove(os_tcb_t **list) {
    if (*list == NULL) return NULL;
    os_tcb_t *tcb = *list;
    *list = tcb->wait_next;
    tcb->wait_next = NULL;
    return tcb;
}

/* --- Semaphore Implementation --- */

void OS_SemInit(os_sem_t *sem, uint32_t initial_count, uint32_t max_count) {
    if (sem == NULL) return;
    sem->count = initial_count;
    sem->max_count = max_count;
    sem->wait_list = NULL;
}

os_status_t OS_SemWait(os_sem_t *sem) {
    OS_ENTER_CRITICAL();
    
    if (sem->count > 0) {
        sem->count--;
        OS_EXIT_CRITICAL();
        return OS_OK;
    }

    /* No resources available, block the task */
    currentTCB->state = TASK_STATE_BLOCKED;
    currentTCB->sleep_ticks = 0; // Infinite wait for now
    wait_list_add(&sem->wait_list, currentTCB);
    
    OS_EXIT_CRITICAL();

    /* Trigger scheduler */
    OS_TRIGGER_PENDSV();
    
    return OS_OK;
}

void OS_SemPost(os_sem_t *sem) {
    OS_ENTER_CRITICAL();

    os_tcb_t *waiting_task = wait_list_remove(&sem->wait_list);
    
    if (waiting_task != NULL) {
        /* Unblock the task */
        waiting_task->state = TASK_STATE_READY;
        OS_EXIT_CRITICAL();
        OS_TRIGGER_PENDSV();
    } else {
        if (sem->count < sem->max_count) {
            sem->count++;
        }
        OS_EXIT_CRITICAL();
    }
}

/* --- Mutex Implementation --- */

void OS_MutexInit(os_mutex_t *mutex) {
    if (mutex == NULL) return;
    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->wait_list = NULL;
}

os_status_t OS_MutexLock(os_mutex_t *mutex) {
    OS_ENTER_CRITICAL();

    if (mutex->owner == NULL) {
        mutex->owner = currentTCB;
        mutex->lock_count = 1;
        OS_EXIT_CRITICAL();
        return OS_OK;
    }
    
    if (mutex->owner == currentTCB) {
        /* Recursive lock */
        mutex->lock_count++;
        OS_EXIT_CRITICAL();
        return OS_OK;
    }

    /* Mutex owned by another task, block */
    currentTCB->state = TASK_STATE_BLOCKED;
    currentTCB->sleep_ticks = 0;
    wait_list_add(&mutex->wait_list, currentTCB);
    
    OS_EXIT_CRITICAL();
    OS_TRIGGER_PENDSV();
    
    return OS_OK;
}

void OS_MutexUnlock(os_mutex_t *mutex) {
    OS_ENTER_CRITICAL();

    if (mutex->owner != currentTCB) {
        OS_EXIT_CRITICAL();
        return;
    }

    mutex->lock_count--;
    if (mutex->lock_count > 0) {
        /* Still locked (recursive) */
        OS_EXIT_CRITICAL();
        return;
    }

    /* Fully unlocked, check for waiting tasks */
    os_tcb_t *waiting_task = wait_list_remove(&mutex->wait_list);
    if (waiting_task != NULL) {
        mutex->owner = waiting_task;
        mutex->lock_count = 1;
        waiting_task->state = TASK_STATE_READY;
        OS_EXIT_CRITICAL();
        OS_TRIGGER_PENDSV();
    } else {
        mutex->owner = NULL;
        OS_EXIT_CRITICAL();
    }
}
