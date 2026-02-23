#include "os_kernel.h"

/* ARM Cortex-M System Control Block registers */
#define SCB_ICSR            (*((volatile uint32_t *)0xE000ED04))
#define SCB_ICSR_PENDSVSET_BIT  (1UL << 28)

/* We need the global pointer to the current task */
extern os_tcb_t *currentTCB;

void OS_Delay(uint32_t ticks) {
    if (ticks == 0) return;

    /*
     * Enter a critical section to prevent an interrupt from occurring
     * while we are modifying the TCB.
     */
    __asm("CPSID I");

    currentTCB->sleep_ticks = ticks;
    currentTCB->state = TASK_STATE_BLOCKED;

    /* Exit the critical section */
    __asm("CPSIE I");

    /*
     * Trigger a PendSV interrupt to switch to another task immediately.
     * It makes no sense to continue execution if the task has just been blocked.
     */
    SCB_ICSR = SCB_ICSR_PENDSVSET_BIT;
}

/**
 * @brief Updates the sleep counters for all tasks.
 *
 * This function is called by the SysTick handler at every system tick.
 * It iterates through the list of tasks and decrements the sleep counter
 * for any task that is in the BLOCKED state. If a counter reaches zero,
 * the task's state is changed back to READY.
 */
void OS_Time_Update(void) {
    // This function is called from an ISR, so we can assume atomicity for now
    // regarding the list traversal, as PendSV has a lower priority.
    os_tcb_t *temp = currentTCB;
    if (temp == NULL) return;

    /* Traverse the circular list of tasks */
    do {
        if (temp->state == TASK_STATE_BLOCKED) {
            if (temp->sleep_ticks > 0) {
                temp->sleep_ticks--;
            }

            /* If the sleep time has expired, the task becomes ready */
            if (temp->sleep_ticks == 0) {
                temp->state = TASK_STATE_READY;
            }
        }
        temp = temp->next;
    } while (temp != currentTCB);
}