#include "os_kernel.h"
#include "os_time.h"

/* ARM Cortex-M System Control Block registers */
#define SCB_ICSR            (*((volatile uint32_t *)0xE000ED04))
#define SCB_ICSR_PENDSVSET_BIT  (1UL << 28)

/* We need the global pointer to the current task */
extern os_tcb_t *currentTCB;

void OS_Internal_Delay(uint32_t ticks) {
    if (ticks == 0) return;

    OS_ENTER_CRITICAL();

    currentTCB->sleep_ticks = ticks;
    currentTCB->state = OS_TASK_STATE_SLEEPING;

    OS_EXIT_CRITICAL();

    /* Trigger a PendSV interrupt to switch to another task immediately */
    SCB_ICSR = SCB_ICSR_PENDSVSET_BIT;
}

/**
 * Updates the sleep counters for all tasks.
 * Called by the SysTick handler at every system tick.
 */
void OS_Time_Update(void) {
    os_tcb_t *temp = currentTCB;
    if (temp == NULL) return;

    /* Traverse the circular list of tasks */
    do {
        if (temp->state == OS_TASK_STATE_SLEEPING && temp->sleep_ticks > 0) {
            temp->sleep_ticks--;

            if (temp->sleep_ticks == 0) {
                temp->state = OS_TASK_STATE_READY;
            }
        }
        temp = temp->next;
    } while (temp != currentTCB);
}
