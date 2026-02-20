#include "os_kernel.h"

#define SCB_ICSR            (*((volatile uint32_t *)0xE000ED04))
#define SCB_ICSR_PENDSVSET_BIT  (1UL << 28)

extern os_tcb_t *currentTCB;

/* Put the current task to sleep for a number of ticks */
void OS_Delay(uint32_t ticks) {
    if (ticks == 0) return;

    /* Disable interrupts for TCB modification */
    __asm("CPSID I");

    currentTCB->sleep_ticks = ticks;
    currentTCB->state = TASK_STATE_BLOCKED;

    __asm("CPSIE I");

    /* Trigger context switch */
    SCB_ICSR = SCB_ICSR_PENDSVSET_BIT;
}

/* Update sleep timers for all blocked tasks */
void OS_Time_Update(void) {
    os_tcb_t *temp = currentTCB;
    if (temp == NULL) return;

    /* Traverse circular list */
    do {
        if (temp->state == TASK_STATE_BLOCKED) {
            if (temp->sleep_ticks > 0) {
                temp->sleep_ticks--;
            }

            /* Wake up task if timer expired */
            if (temp->sleep_ticks == 0) {
                temp->state = TASK_STATE_READY;
            }
        }
        temp = temp->next;
    } while (temp != currentTCB);
}
