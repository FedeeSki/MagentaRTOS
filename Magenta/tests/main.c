#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "magenta.h"
#include "os_syscall.h"

#define BLUE_LED 15
#define RED_LED  14

/* ------------------------------------------------------------------
 * TASK 1: Heartbeat (Blue)
 * Indica che lo scheduler è vivo e i task User Mode corrono.
 * ------------------------------------------------------------------ */
OS_TASK_STACK_DEFINE(Task_Heart, 512);
OS_TASK_TCB_DEFINE(Task_Heart);
void Task_Heart_Func(void) {
    while(1) {
        gpio_put(BLUE_LED, 1);
        OS_Delay(100);
        gpio_put(BLUE_LED, 0);
        OS_Delay(100);
    }
}

/* ------------------------------------------------------------------
 * TASK 2: Logger
 * Dimostra l'uso sicuro delle System Call.
 * ------------------------------------------------------------------ */
OS_TASK_STACK_DEFINE(Task_Log, 512);
OS_TASK_TCB_DEFINE(Task_Log);
void Task_Log_Func(void) {
    while(1) {
        OS_SafePrintf("[LOG] Sistema operativo MagentaRTOS in esecuzione...\n");
        OS_Delay(2000);
    }
}

/* ------------------------------------------------------------------
 * TASK 3: Safe Worker
 * Un task che non fa nulla di pericoloso.
 * ------------------------------------------------------------------ */
OS_TASK_STACK_DEFINE(Task_Safe, 512);
OS_TASK_TCB_DEFINE(Task_Safe);
void Task_Safe_Func(void) {
    volatile uint32_t work = 0;
    while(1) {
        work++;
        OS_Yield();
    }
}

/* ------------------------------------------------------------------
 * TASK 4: The Villain (MPU Violator)
 * Dopo 10 secondi, tenterà di accedere a una periferica NON mappata.
 * ------------------------------------------------------------------ */
OS_TASK_STACK_DEFINE(Task_Villain, 512);
OS_TASK_TCB_DEFINE(Task_Villain);
void Task_Villain_Func(void) {
    OS_Delay(10000); 
    OS_SafePrintf("[VILLAIN] Tentativo di accesso a periferica PROIBITA (Timer)...\n");
    
    /* Timer Register a 0x40054000 non è nelle Region MPU 0-4! */
    volatile uint32_t *prohibited_periph = (uint32_t *)0x40054000;
    uint32_t val = *prohibited_periph; 
    
    OS_SafePrintf("[VILLAIN] Se leggi questo, la MPU ha fallito! Val: %u\n", val);
    while(1) OS_Yield();
}

int main() {
    stdio_init_all();
    
    gpio_init(BLUE_LED);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);

    sleep_ms(3000);
    printf("\n--- MagentaRTOS: Security & Multi-Task Test ---\n");

    OS_Init();

    /* Creazione Task - Ora i nomi coincidono con le DEFINE sopra */
    OS_TASK_CREATE_STATIC(Task_Heart,   Task_Heart_Func,   512, 10);
    OS_TASK_CREATE_STATIC(Task_Log,     Task_Log_Func,     512, 5);
    OS_TASK_CREATE_STATIC(Task_Safe,    Task_Safe_Func,    512, 1);
    OS_TASK_CREATE_STATIC(Task_Villain, Task_Villain_Func, 512, 1);

    printf("[Kernel] Avvio Test di Sicurezza...\n");
    OS_Start();

    return 0;
}
