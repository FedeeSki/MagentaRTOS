#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "os_kernel.h"
#include <stdio.h>

/* LED pins for visual debugging */
#define BLUE_LED 15
#define RED_LED  14

/* Stack size for test tasks */
#define TEST_STACK_SIZE 1024

/* Global counters to track task activity */
volatile uint32_t t1_count = 0;
volatile uint32_t t2_count = 0;
volatile uint32_t t3_count = 0;
volatile uint32_t t4_count = 0;

/* TCBs and Stacks */
static os_tcb_t tcb1, tcb2, tcb3, tcb4;
static os_stack_t s1[TEST_STACK_SIZE], s2[TEST_STACK_SIZE], s3[TEST_STACK_SIZE], s4[TEST_STACK_SIZE];

/* Task 1: Start indicator (Blue LED) */
void Task1_StartIndicator(void) {
    printf("[TASK 1] Start Indicator Online (Blue LED)\n");
    while (1) {
        gpio_put(BLUE_LED, 1);
        OS_Delay(250);
        gpio_put(BLUE_LED, 0);
        OS_Delay(250);
        t1_count++;
    }
}

/* Task 2: Workload (Integer) */
void Task2_Math(void) {
    printf("[TASK 2] Integer Worker Online\n");
    volatile uint32_t val = 0;
    while (1) {
        for(int i=0; i<10000; i++) {
            val = (val + i) % 1000;
        }
        t2_count++;
        OS_Delay(100);
    }
}

/* Task 3: Workload (FPU) */
void Task3_FPU(void) {
    printf("[TASK 3] FPU Worker Online\n");
    float f = 1.0f;
    while (1) {
        for(int i=0; i<5000; i++) {
            f = (f * 1.001f) / 0.999f;
            if (f > 1000.0f) f = 1.0f;
        }
        t3_count++;
        OS_Delay(100);
    }
}

/* Task 4: End indicator (Red LED) + Console Monitor */
void Task4_EndIndicator(void) {
    printf("[TASK 4] End Indicator Online (Red LED)\n");
    while (1) {
        gpio_put(RED_LED, 1);
        OS_Delay(250);
        gpio_put(RED_LED, 0);
        OS_Delay(250);
        t4_count++;
        
        /* Print heartbeat every 4 toggles (approx every 2 seconds) */
        if (t4_count % 4 == 0) {
            printf("\n--- RTOS MONITOR ---\n");
            printf("T1 (BLUE): %lu cycles\n", t1_count);
            printf("T2 (INT) : %lu cycles\n", t2_count);
            printf("T3 (FPU) : %lu cycles\n", t3_count);
            printf("T4 (RED) : %lu cycles\n", t4_count);
            printf("--------------------\n");
        }
    }
}

int main(void) {
    /* Standard Pico SDK initialization */
    stdio_init_all();
    
    /* Setup LED GPIOs */
    gpio_init(BLUE_LED);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);

    /* Small delay to let UART connect */
    sleep_ms(2000);
    printf("--- MagentaRTOS Layer 1 Stress Test ---\n");

    /* Initialize Kernel */
    OS_Init();

    /* Create tasks */
    OS_TaskCreate(&tcb1, Task1_StartIndicator, s1, TEST_STACK_SIZE);
    OS_TaskCreate(&tcb2, Task2_Math, s2, TEST_STACK_SIZE);
    OS_TaskCreate(&tcb3, Task3_FPU, s3, TEST_STACK_SIZE);
    OS_TaskCreate(&tcb4, Task4_EndIndicator, s4, TEST_STACK_SIZE);

    printf("[KERNEL] Starting Scheduler...\n");
    OS_Start();

    while(1);
    return 0;
}
