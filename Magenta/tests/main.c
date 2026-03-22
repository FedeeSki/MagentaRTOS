#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "magenta.h"

#define BLUE_LED 15
#define RED_LED  14

/* Global counters for the Monitor */
volatile uint32_t t1_count = 0;
volatile uint32_t t2_count = 0;
volatile uint32_t t3_count = 0;
volatile uint32_t t4_count = 0;

/* Static Allocation for 4 Tasks */
OS_TASK_STACK_DEFINE(Task1, 512);
OS_TASK_TCB_DEFINE(Task1);

OS_TASK_STACK_DEFINE(Task2, 512);
OS_TASK_TCB_DEFINE(Task2);

OS_TASK_STACK_DEFINE(Task3, 512);
OS_TASK_TCB_DEFINE(Task3);

OS_TASK_STACK_DEFINE(Task4, 1024); /* Larger stack for printf */
OS_TASK_TCB_DEFINE(Task4);


// OS_TASK_STACK_DEFINE(Monster, 150000);
// OS_TASK_TCB_DEFINE(Monster);


/* T1: Blue LED Heartbeat */
void Task1_Blue(void) {
    gpio_init(BLUE_LED);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    while(1) {
        gpio_put(BLUE_LED, 1);
        OS_Delay(250);
        gpio_put(BLUE_LED, 0);
        OS_Delay(250);
        t1_count++;
    }
}

/* T2: Integer Math Workload */
void Task2_IntMath(void) {
    volatile uint32_t val = 0;
    while(1) {
        for(int i=0; i<5000; i++) {
            val = (val + i) % 1000;
        }
        t2_count++;
        OS_Delay(50);
    }
}

/* T3: FPU Math Workload (Virtuosismo) */
void Task3_FPUMath(void) {
    float f = 1.0f;
    while(1) {
        for(int i=0; i<2000; i++) {
            f = (f * 1.001f) / 0.999f;
            if (f > 100.0f) f = 1.0f;
        }
        t3_count++;
        OS_Delay(50);
    }
}

/* T4: Red LED + System Monitor */
void Task4_Monitor(void) {
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);
    while(1) {
        gpio_put(RED_LED, 1);
        OS_Delay(500);
        gpio_put(RED_LED, 0);
        OS_Delay(500);
        t4_count++;

        /* Print Dashboard every 4 cycles (~4 seconds) */
        if(t4_count % 4 == 0) {
            printf("\n--- RTOS MONITOR ---\n");
            printf("T1 (BLUE) : %lu cycles\n", t1_count);
            printf("T2 (INT)  : %lu cycles\n", t2_count);
            printf("T3 (FPU)  : %lu cycles\n", t3_count);
            printf("T4 (RED)  : %lu cycles\n", t4_count);
            printf("--------------------\n");
        }
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000); /* Wait for UART */
    printf("--- MagentaRTOS Dashboard Test ---\n");

    OS_Init();

    /* Create Tasks Statistically */
    OS_TASK_CREATE_STATIC(Task1, Task1_Blue, 512);
    OS_TASK_CREATE_STATIC(Task2, Task2_IntMath, 512);
    OS_TASK_CREATE_STATIC(Task3, Task3_FPUMath, 512);
    OS_TASK_CREATE_STATIC(Task4, Task4_Monitor, 1024);

    // OS_TASK_CREATE_STATIC(Monster, Task1_Blue, 150000);

    printf("[KERNEL] System Ready. Starting Scheduler...\n");
    OS_Start();
    return 0;
}
