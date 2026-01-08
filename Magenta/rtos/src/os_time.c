#include "os_kernel.h"

/* Importiamo le macro e le variabili globali necessarie */
extern os_tcb_t *currentTCB;

/* Definiamo l'indirizzo per prenotare il PendSV (come nel tuo os_port.c) */
#define INT_CTRL_STATE      (*((volatile uint32_t *)0xE000ED04))
#define PENDSVSET           (1 << 28)

/**
 * Mette il task attuale in pausa.
 */
void OS_Delay(uint32_t ticks) {
    if (ticks == 0) return;

    /* Entriamo in sezione critica per evitare che un interrupt modifichi lo stato ora */
    __asm("CPSID I"); 

    currentTCB->sleep_ticks = ticks;
    currentTCB->state = TASK_STATE_BLOCKED;

    /* Usciamo dalla sezione critica */
    __asm("CPSIE I");

    /* Prenotiamo il PendSV usando la tua macro per cambiare task immediatamente.
       Non ha senso continuare l'esecuzione se il task è appena stato bloccato. */
    INT_CTRL_STATE = PENDSVSET;
}

/**
 * Aggiorna i contatori. Viene chiamata dal SysTick.
 */
void OS_Time_Update(void) {
    os_tcb_t *temp = currentTCB;
    if (temp == NULL) return;

    /* Scorriamo la lista circolare dei task */
    do {
        if (temp->state == TASK_STATE_BLOCKED) {
            if (temp->sleep_ticks > 0) {
                temp->sleep_ticks--;
            }
            
            /* Se il tempo è scaduto, il task torna pronto */
            if (temp->sleep_ticks == 0) {
                temp->state = TASK_STATE_READY;
            }
        }
        temp = temp->next;
    } while (temp != currentTCB);
}