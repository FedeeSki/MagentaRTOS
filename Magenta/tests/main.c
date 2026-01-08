#include "pico/stdlib.h"

#include "os_kernel.h" 
#include "os_types.h"   
#include <stdio.h>


#define LED_GP 15

// Definizione degli stack per i task (es. 128 parole da 32 bit = 512 byte)
#define STACK_SIZE 512
os_stack_t stack1[STACK_SIZE];
os_stack_t stack2[STACK_SIZE];

// TCBs
os_tcb_t tcb1;
os_tcb_t tcb2;

// 3. Funzioni dei Task

void Task_Led(void) {
    gpio_init(LED_GP);           // Inizializza GP15 
    gpio_set_dir(LED_GP, GPIO_OUT); // Imposta come output 

    while(1) {
        gpio_put(LED_GP, 1);    // Accende il LED 
        OS_Delay(500);          // Usa il tuo kernel per attendere
        gpio_put(LED_GP, 0);    // Spegne il LED 
        OS_Delay(500);          // Torna nello stato BLOCKED
    }
}

void Task1_Func(void) {
    while(1) {
        // Logica del Task 1 (es. accendi un LED)
        OS_Delay(500); // Dorme per 500 tick
    }
}

void Task2_Func(void) {
    while(1) {
        // Logica del Task 2 (es. invia un messaggio UART)
        OS_Delay(1000); // Dorme per 1000 tick
    }
}

int main(void) {


    stdio_init_all();

    // 1. Inizializza il kernel (crea l'Idle Task)
    OS_Init();

    // 2. Crea i tuoi task utente
    OS_TaskCreate(&tcb1, Task_Led, stack1 , 256);

    // 3. Fai partire il sistema
    OS_Start();

    while(1); // Non raggiunto
}