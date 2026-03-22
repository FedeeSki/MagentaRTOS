#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "magenta.h"

#define BLUE_LED 15
#define RED_LED  14

/* Oggetti di sincronizzazione */
os_sem_t sem_items;
os_mutex_t resource_mutex;

/* Allocazione Statica per i Task di Sincronizzazione */
OS_TASK_STACK_DEFINE(Producer, 1024);
OS_TASK_TCB_DEFINE(Producer);

OS_TASK_STACK_DEFINE(Consumer, 1024);
OS_TASK_TCB_DEFINE(Consumer);

OS_TASK_STACK_DEFINE(Mutex1, 1024);
OS_TASK_TCB_DEFINE(Mutex1);

OS_TASK_STACK_DEFINE(Mutex2, 1024);
OS_TASK_TCB_DEFINE(Mutex2);

void ProducerTask(void) {
    gpio_init(BLUE_LED);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    while (1) {
        OS_Delay(1000); 
        gpio_put(BLUE_LED, 1);
        OS_SemPost(&sem_items);
        OS_Delay(100); 
        gpio_put(BLUE_LED, 0);
        printf("[PRODUCER] Segnale inviato (Semaforo)\n");
    }
}

void ConsumerTask(void) {
    while (1) {
        OS_SemWait(&sem_items);
        printf("[CONSUMER] Elemento consumato!\n");
    }
}

void MutexTask1(void) {
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);
    while (1) {
        OS_MutexLock(&resource_mutex);
        gpio_put(RED_LED, 1);
        printf("[MUTEX 1] Risorsa acquisita\n");
        OS_Delay(500);
        OS_MutexUnlock(&resource_mutex);
        gpio_put(RED_LED, 0);
        OS_Delay(200);
    }
}

void MutexTask2(void) {
    while (1) {
        OS_MutexLock(&resource_mutex);
        gpio_put(RED_LED, 1);
        printf("[MUTEX 2] Risorsa acquisita\n");
        OS_Delay(300);
        OS_MutexUnlock(&resource_mutex);
        gpio_put(RED_LED, 0);
        OS_Delay(400);
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("--- MagentaRTOS Layer 2: Sync LED Test ---\n");

    OS_Init();

    /* Inizializzazione primitive */
    OS_SemInit(&sem_items, 0, 10);
    OS_MutexInit(&resource_mutex);

    /* Creazione Task */
    OS_TASK_CREATE_STATIC(Producer, ProducerTask, 1024);
    OS_TASK_CREATE_STATIC(Consumer, ConsumerTask, 1024);
    OS_TASK_CREATE_STATIC(Mutex1, MutexTask1, 1024);
    OS_TASK_CREATE_STATIC(Mutex2, MutexTask2, 1024);

    printf("[KERNEL] Avvio Scheduler...\n");
    OS_Start();

    return 0;
}
