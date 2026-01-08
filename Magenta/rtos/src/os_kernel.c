#include "os_kernel.h"
#include <stddef.h>

/* --- Definizioni Bare Metal per la gestione interrupt --- */
static inline void __disable_irq(void) {
    __asm volatile ("CPSID I" : : : "memory");
}

static inline void __enable_irq(void) {
    __asm volatile ("CPSIE I" : : : "memory");
}

/* Puntatore al task che sta girando */
os_tcb_t *currentTCB = NULL;


/* Stack per il task Idle (dimensione minima) */
#define IDLE_STACK_SIZE 256
static os_stack_t idle_stack[IDLE_STACK_SIZE];
static os_tcb_t idleTCB;

/* Funzione del task Idle: un ciclo infinito che non fa nulla */
void OS_IdleTask(void) {
    while(1) {
        // Opzionale: istruzione per risparmiare energia
        __asm("WFI"); //Wait for interrupt
    }
}

void OS_Init(void) {
    currentTCB = NULL;
    
    // Creiamo il task Idle che sarà sempre presente nella lista
    OS_TaskCreate(&idleTCB, OS_IdleTask, idle_stack, IDLE_STACK_SIZE);
}


void OS_Scheduler(void) {
    // Partiamo dal task successivo a quello attuale
    os_tcb_t *nextTCB = currentTCB->next;

    /* Cerchiamo il prossimo task che sia in stato READY.
       Grazie all'Idle Task creato in OS_Init, troveremo SEMPRE 
       almeno un task pronto (l'Idle Task stesso). */
    while (nextTCB->state != TASK_STATE_READY) {
        nextTCB = nextTCB->next;
    }

    currentTCB = nextTCB;
}




extern os_stack_t* OS_Port_StackInit(void (*task_func)(void), os_stack_t *stack_top);

os_status_t OS_TaskCreate(os_tcb_t *tcb, void (*task_func)(void), os_stack_t *stack_base, uint32_t stack_size) {
    // 1. Controllo parametri
    if (tcb == NULL || task_func == NULL || stack_base == NULL || stack_size == 0) {
        return OS_ERR_PARAM;
    }

    // 2. Calcolo stack_top con allineamento preventivo
    // Sommiamo stack_size e poi forziamo l'allineamento a 8 byte verso il basso
    uintptr_t top_address = (uintptr_t)stack_base + (stack_size * sizeof(os_stack_t));
    os_stack_t *stack_top = (os_stack_t *)(top_address & ~0x7);

    // 3. Inizializzazione fisica dello stack
    tcb->stackPtr = OS_Port_StackInit(task_func, stack_top);
    
    tcb->state = TASK_STATE_READY;
    tcb->sleep_ticks = 0;
    tcb->task_id = 0;

    // 4. Inserimento nella lista circolare (Sezione Critica Protetta)
   __disable_irq(); // Disabilita interrupt

    if (currentTCB == NULL) {
        currentTCB = tcb;
        tcb->next = tcb;
    } else {
        tcb->next = currentTCB->next;
        currentTCB->next = tcb;
    }

    __enable_irq(); // Riabilita interrupt

    return OS_OK;
}


extern void OS_Launch_First_Task(os_tcb_t **tcb_ptr);

extern void OS_Port_InitTick(uint32_t tick_ms);

os_status_t OS_Start(void) {
    // 1. Controllo di sicurezza: abbiamo almeno un task?
    if (currentTCB == NULL) {
        return OS_ERR_PARAM;
    }

    // 2. Disabilita gli interrupt globali durante la fase di avvio
    __disable_irq();

    // 3. Inizializza il System Tick (es. 1ms) 
    // Questa funzione è definita nel tuo os_port.c
    OS_Port_InitTick(1); 

    /* * 4. Lancio del primo task.
     * Invece di provare a saltare al PC manualmente in C (che è rischioso),
     * chiamiamo la funzione Assembly che hai scritto in os_port_asm.s.
     * * Passiamo l'indirizzo di currentTCB.
     */
    OS_Launch_First_Task(&currentTCB);

    /* * Non arriveremo mai qui. 
     * OS_Launch_First_Task cambia lo stack e salta alla funzione del task.
     */
    return OS_OK; 
}













