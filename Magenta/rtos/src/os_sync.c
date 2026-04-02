#include "os_sync.h"
#include <stddef.h>

extern os_tcb_t *currentTCB;

/* Internal: Add a task to a FIFO wait list (used by Semaphores) */
static void wait_list_add_fifo(os_tcb_t **list, os_tcb_t *tcb) {
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

/* Internal: Add a task to a Priority-ordered wait list (used by Mutexes) */
static void wait_list_add_priority(os_tcb_t **list, os_tcb_t *tcb) {
    if (*list == NULL || tcb->priority > (*list)->priority) {
        tcb->wait_next = *list;
        *list = tcb;
    } else {
        os_tcb_t *current = *list;
        while (current->wait_next != NULL && current->wait_next->priority >= tcb->priority) {
            current = current->wait_next;
        }
        tcb->wait_next = current->wait_next;
        current->wait_next = tcb;
    }
}

/* Internal: Remove the first task from any wait list */
static os_tcb_t* wait_list_remove(os_tcb_t **list) {
    if (*list == NULL) return NULL;
    os_tcb_t *tcb = *list;
    *list = tcb->wait_next;
    tcb->wait_next = NULL;
    return tcb;
}

/* Internal: Recalculate task priority based on base_priority and all owned mutexes */
static void OS_Mutex_Recalculate_Priority(os_tcb_t *tcb) {
    os_priority_t highest_prio = tcb->base_priority;
    os_mutex_t *mutex = tcb->owned_mutex_list;

    while (mutex != NULL) {
        if (mutex->wait_list != NULL && mutex->wait_list->priority > highest_prio) {
            highest_prio = mutex->wait_list->priority;
        }
        mutex = mutex->next_owned;
    }
    tcb->priority = highest_prio;
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
    currentTCB->state = TASK_STATE_BLOCKED;
    currentTCB->sleep_ticks = 0;
    wait_list_add_fifo(&sem->wait_list, currentTCB);
    OS_EXIT_CRITICAL();
    OS_TRIGGER_PENDSV();
    return OS_OK;
}

void OS_SemPost(os_sem_t *sem) {
    OS_ENTER_CRITICAL();
    os_tcb_t *waiting_task = wait_list_remove(&sem->wait_list);
    if (waiting_task != NULL) {
        waiting_task->state = TASK_STATE_READY;
        OS_EXIT_CRITICAL();
        OS_TRIGGER_PENDSV();
    } else {
        if (sem->count < sem->max_count) sem->count++;
        OS_EXIT_CRITICAL();
    }
}

/* --- Mutex Implementation with Multiple Priority Inheritance --- */

void OS_MutexInit(os_mutex_t *mutex) {
    if (mutex == NULL) return;
    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->wait_list = NULL;
    mutex->next_owned = NULL;
}

os_status_t OS_MutexLock(os_mutex_t *mutex) {
    OS_ENTER_CRITICAL();

    if (mutex->owner == NULL) {
        mutex->owner = currentTCB;
        mutex->lock_count = 1;
        /* Add mutex to task's owned list */
        mutex->next_owned = currentTCB->owned_mutex_list;
        currentTCB->owned_mutex_list = mutex;
        OS_EXIT_CRITICAL();
        return OS_OK;
    }
    
    if (mutex->owner == currentTCB) {
        mutex->lock_count++;
        OS_EXIT_CRITICAL();
        return OS_OK;
    }

    /* Block current task until mutex is released */
    currentTCB->state = TASK_STATE_BLOCKED;
    currentTCB->sleep_ticks = 0;
    wait_list_add_priority(&mutex->wait_list, currentTCB);
    
    /* Propagate priority to the owner */
    if (currentTCB->priority > mutex->owner->priority) {
        mutex->owner->priority = currentTCB->priority;
    }
    
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
        OS_EXIT_CRITICAL();
        return;
    }

    /* Remove mutex from the task's owned list */
    os_mutex_t **indirect = &currentTCB->owned_mutex_list;
    while ((*indirect) != mutex) {
        indirect = &(*indirect)->next_owned;
    }
    *indirect = mutex->next_owned;
    mutex->next_owned = NULL;

    /* Re-evaluate current task's priority */
    OS_Mutex_Recalculate_Priority(currentTCB);

    /* Pass ownership to the first task in the priority-ordered wait list */
    os_tcb_t *waiting_task = wait_list_remove(&mutex->wait_list);
    if (waiting_task != NULL) {
        mutex->owner = waiting_task;
        mutex->lock_count = 1;
        /* Add mutex to the new owner's list */
        mutex->next_owned = waiting_task->owned_mutex_list;
        waiting_task->owned_mutex_list = mutex;
        
        waiting_task->state = TASK_STATE_READY;
        OS_EXIT_CRITICAL();
        OS_TRIGGER_PENDSV();
    } else {
        mutex->owner = NULL;
        OS_EXIT_CRITICAL();
    }
}
