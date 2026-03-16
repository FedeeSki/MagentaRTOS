#include "pico/stdlib.h"
#include "os_kernel.h"
#include "os_sync.h"
#include <stdio.h>

#define TEST_STACK_SIZE 1024

static os_tcb_t tcb_producer, tcb_consumer, tcb_mutex1, tcb_mutex2;
static os_stack_t s_p[TEST_STACK_SIZE], s_c[TEST_STACK_SIZE], s_m1[TEST_STACK_SIZE], s_m2[TEST_STACK_SIZE];

os_sem_t sem_items;
os_mutex_t resource_mutex;

volatile uint32_t shared_resource = 0;
volatile uint32_t produced_count = 0;
volatile uint32_t consumed_count = 0;

/* Producer Task: Signals a semaphore */
void ProducerTask(void) {
    printf("[PRODUCER] Started\n");
    while (1) {
        OS_Delay(1000); // Produce every 1s
        produced_count++;
        printf("[PRODUCER] Item produced (%lu), signaling semaphore...\n", produced_count);
        OS_SemPost(&sem_items);
    }
}

/* Consumer Task: Waits on a semaphore */
void ConsumerTask(void) {
    printf("[CONSUMER] Started\n");
    while (1) {
        printf("[CONSUMER] Waiting for item...\n");
        OS_SemWait(&sem_items);
        consumed_count++;
        printf("[CONSUMER] Item consumed (%lu)!\n", consumed_count);
    }
}

/* Mutex Task 1: Competes for a shared resource */
void MutexTask1(void) {
    printf("[MUTEX 1] Started\n");
    while (1) {
        OS_MutexLock(&resource_mutex);
        printf("[MUTEX 1] Resource locked, incrementing...\n");
        shared_resource++;
        OS_Delay(500);
        printf("[MUTEX 1] Shared resource is now: %lu. Unlocking...\n", shared_resource);
        OS_MutexUnlock(&resource_mutex);
        OS_Delay(200);
    }
}

/* Mutex Task 2: Competes for a shared resource */
void MutexTask2(void) {
    printf("[MUTEX 2] Started\n");
    while (1) {
        OS_MutexLock(&resource_mutex);
        printf("[MUTEX 2] Resource locked, incrementing...\n");
        shared_resource++;
        OS_Delay(300);
        printf("[MUTEX 2] Shared resource is now: %lu. Unlocking...\n", shared_resource);
        OS_MutexUnlock(&resource_mutex);
        OS_Delay(400);
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("--- MagentaRTOS Layer 2: IPC & Sync Test ---\n");

    OS_Init();

    /* Initialize Sync Primitives */
    OS_SemInit(&sem_items, 0, 10);
    OS_MutexInit(&resource_mutex);

    /* Create tasks */
    OS_TaskCreate(&tcb_producer, ProducerTask, s_p, TEST_STACK_SIZE);
    OS_TaskCreate(&tcb_consumer, ConsumerTask, s_c, TEST_STACK_SIZE);
    OS_TaskCreate(&tcb_mutex1, MutexTask1, s_m1, TEST_STACK_SIZE);
    OS_TaskCreate(&tcb_mutex2, MutexTask2, s_m2, TEST_STACK_SIZE);

    printf("[KERNEL] Starting Scheduler...\n");
    OS_Start();

    while(1);
    return 0;
}
