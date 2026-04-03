#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "magenta.h"

#define BLUE_LED 15
#define RED_LED  14

/* Helper per ritardare l'esecuzione in User Mode senza chiamare funzioni Kernel.
   In User Mode non possiamo accedere a variabili globali o usare OS_Delay 
   (che disabilita gli interrupt). Usiamo cicli NOP locali. */
void user_busy_wait(uint32_t iterations) {
    for(volatile uint32_t i=0; i<iterations; i++) {
        __asm volatile("nop");
    }
}

/* ------------------------------------------------------------------
 * TASK 1: LED Blink (Testa l'accesso alla Region 2 - SIO)
 * ------------------------------------------------------------------ */
OS_TASK_STACK_DEFINE(Task1_LED, 512);
OS_TASK_TCB_DEFINE(Task1_LED);
void Task1_Func(void) {
    /* Nota: Usiamo solo variabili locali (nello Stack, Region 4). 
       Nessun printf() perché la libreria C usa variabili globali in RAM! */
    while(1) {
        gpio_put(BLUE_LED, 1);
        user_busy_wait(1000000);
        gpio_put(BLUE_LED, 0);
        user_busy_wait(1000000);
    }
}

/* ------------------------------------------------------------------
 * TASK 2: LED Rosso (Testa l'accesso alla Region 2 - SIO con freq diversa)
 * ------------------------------------------------------------------ */
OS_TASK_STACK_DEFINE(Task2_LED, 512);
OS_TASK_TCB_DEFINE(Task2_LED);
void Task2_Func(void) {
    while(1) {
        gpio_put(RED_LED, 1);
        user_busy_wait(2000000);
        gpio_put(RED_LED, 0);
        user_busy_wait(2000000);
    }
}

/* ------------------------------------------------------------------
 * TASK 3: FPU Math (Testa l'accesso alla Region 4 - Task Stack)
 * ------------------------------------------------------------------ */
OS_TASK_STACK_DEFINE(Task3_Math, 512);
OS_TASK_TCB_DEFINE(Task3_Math);
void Task3_Func(void) {
    /* Il calcolo float forza il salvataggio dei registri S16-S31 nel Context Switch */
    volatile float f = 1.0f;
    while(1) {
        for(int i=0; i<1000; i++) {
            f = (f * 1.01f) / 0.99f;
        }
    }
}

int main() {
    stdio_init_all();
    
    /* Inizializzazione Hardware Globale sicura (SUDO Mode) */
    gpio_init(BLUE_LED);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    gpio_put(BLUE_LED, 0);
    
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_put(RED_LED, 0);

    sleep_ms(3000);

    OS_Init();

    /* 
     * Creiamo i task con la STESSA PRIORITÀ (10).
     * Dato che i task usano "busy waits" e non cedono mai volontariamente la CPU,
     * mettendoli alla pari costringiamo lo scheduler a usare il ROUND-ROBIN.
     * La CPU passerà da uno all'altro ogni 1ms grazie al SysTick (Preemption).
     */
    OS_TASK_CREATE_STATIC(Task1_LED,  Task1_Func, 512, 10);
    OS_TASK_CREATE_STATIC(Task2_LED,  Task2_Func, 512, 10);
    OS_TASK_CREATE_STATIC(Task3_Math, Task3_Func, 512, 10);

    OS_Start();
    return 0;
}
