#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "magenta.h"

#define BLUE_LED 15
#define RED_LED  14

volatile uint32_t t1_cycles = 0, t2_cycles = 0, t3_cycles = 0, t4_cycles = 0;

/* T1: Alta Priorità (150) - Risposta rapida I/O */
OS_TASK_STACK_DEFINE(T1_Critical, 512);
OS_TASK_TCB_DEFINE(T1_Critical);
void Task1_Critical(void) {
    gpio_init(BLUE_LED);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    while(1) {
        gpio_put(BLUE_LED, 1);
        OS_Delay(100);
        gpio_put(BLUE_LED, 0);
        OS_Delay(100);
        t1_cycles++;
    }
}

/* T2: Media Priorità (100) - Logica di controllo */
OS_TASK_STACK_DEFINE(T2_Control, 512);
OS_TASK_TCB_DEFINE(T2_Control);
void Task2_Control(void) {
    volatile uint32_t x = 0;
    while(1) {
        for(int i=0; i<5000; i++) x = (x + i) % 777;
        t2_cycles++;
        OS_Delay(50);
    }
}

/* T3: Bassa Priorità (50) - Calcoli pesanti in background */
OS_TASK_STACK_DEFINE(T3_Background, 512);
OS_TASK_TCB_DEFINE(T3_Background);
void Task3_Background(void) {
    float f = 1.1f;
    while(1) {
        for(int i=0; i<2000; i++) f = (f * 1.01f) / 0.99f;
        t3_cycles++;
        OS_Delay(20);
    }
}

/* T4: Priorità Massima (200) - Monitor di Sistema */
OS_TASK_STACK_DEFINE(T4_Monitor, 1024);
OS_TASK_TCB_DEFINE(T4_Monitor);
void Task4_Monitor(void) {
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);
    while(1) {
        gpio_put(RED_LED, 1);
        OS_Delay(2000);
        gpio_put(RED_LED, 0);
        OS_Delay(2000);
        t4_cycles++;
        
        printf("\n--- MagentaRTOS Real-Time Dashboard ---\n");
        printf("T1 (Critical) [P:150]: %lu\n", t1_cycles);
        printf("T2 (Control)  [P:100]: %lu\n", t2_cycles);
        printf("T3 (Backgr)   [P: 50]: %lu\n", t3_cycles);
        printf("T4 (Monitor)  [P:200]: %lu\n", t4_cycles);
        printf("--------------------------------------\n");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    OS_Init();
    
    OS_TASK_CREATE_STATIC(T1_Critical,   Task1_Critical,   512,  150);
    OS_TASK_CREATE_STATIC(T2_Control,    Task2_Control,    512,  100);
    OS_TASK_CREATE_STATIC(T3_Background, Task3_Background, 512,   50);
    OS_TASK_CREATE_STATIC(T4_Monitor,    Task4_Monitor,    1024, 200);
    
    printf("[KERNEL] System Ready. Starting Scheduler...\n");
    OS_Start();
    return 0;
}
