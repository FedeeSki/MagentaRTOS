#include "pico/stdlib.h"
#include <stdio.h>
#include "magenta.h"

os_mutex_t shared_mutex;

/* Helper per stampare lo stato del task */
void print_task_status(const char* name, os_priority_t p, os_priority_t bp) {
    printf("[%s] Prio Corrente: %u (Base: %u)\n", name, p, bp);
}

/* Task L: Priorità 10 (Minima) */
OS_TASK_STACK_DEFINE(TaskL, 1024);
OS_TASK_TCB_DEFINE(TaskL);
void TaskL_Func(void) {
    extern os_tcb_t *currentTCB;
    while(1) {
        printf("[L] Tento di prendere il Mutex...\n");
        OS_MutexLock(&shared_mutex);
        print_task_status("L", currentTCB->priority, currentTCB->base_priority);
        printf("[L] Mutex preso. Simulo lavoro critico per 5 secondi...\n");
        
        /* 
         * Durante questi 5 secondi, se Task H si sveglia e prova a prendere il mutex,
         * vedrai la priorità di L salire a 100 nei log successivi o tramite monitor.
         */
        OS_Delay(5000); 
        
        print_task_status("L", currentTCB->priority, currentTCB->base_priority);
        printf("[L] Rilascio Mutex.\n");
        OS_MutexUnlock(&shared_mutex);
        OS_Delay(1000);
    }
}

/* Task M: Priorità 50 (Media) - Il "Disturbatore" */
OS_TASK_STACK_DEFINE(TaskM, 1024);
OS_TASK_TCB_DEFINE(TaskM);
void TaskM_Func(void) {
    OS_Delay(2000); // Aspetta che L sia dentro
    while(1) {
        printf("[M] Task Medio (50) in esecuzione. Se L non eredita 100, H non correrà mai!\n");
        /* Stress CPU simulato con delay corto per non bloccare la UART */
        OS_Delay(1000);
    }
}

/* Task H: Priorità 100 (Alta) */
OS_TASK_STACK_DEFINE(TaskH, 1024);
OS_TASK_TCB_DEFINE(TaskH);
void TaskH_Func(void) {
    OS_Delay(3000); // Aspetta che L abbia il mutex e M stia rompendo
    while(1) {
        printf("[H] Task Alta (100) vuole il Mutex. Provo a prenderlo...\n");
        OS_MutexLock(&shared_mutex);
        printf("[H] SUCCESSO! Ho il Mutex.\n");
        OS_MutexUnlock(&shared_mutex);
        OS_Delay(2000);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("\n--- MagentaRTOS Priority Inheritance Stress Test ---\n");
    
    OS_Init();
    OS_MutexInit(&shared_mutex);
    
    OS_TASK_CREATE_STATIC(TaskL, TaskL_Func, 1024, 10);
    OS_TASK_CREATE_STATIC(TaskM, TaskM_Func, 1024, 50);
    OS_TASK_CREATE_STATIC(TaskH, TaskH_Func, 1024, 100);
    
    OS_Start();
    return 0;
}
