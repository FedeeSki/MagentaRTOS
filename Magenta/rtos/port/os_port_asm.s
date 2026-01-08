.syntax unified
.cpu cortex-m33
.thumb

.global PendSV_Handler
.extern currentTCB
.extern OS_Scheduler

.section .text

/* -------------------------------------------------------------------------
   PendSV_Handler: Il gestore del cambio di contesto.
   Viene innescato dal SysTick quando scade il tempo del task attuale.
   ------------------------------------------------------------------------- */
PendSV_Handler:
    /* 1. DISABILITAZIONE INTERRUPT (Sezione Critica) */
    CPSID   I               

    /* 2. SALVATAGGIO CONTESTO TASK USCENTE */
    MRS     R0, PSP         /* R0 = Process Stack Pointer del task attuale */
    
    /* STMDB: Store Multiple Decrement Before. 
       Salva i registri R4-R11 nello stack del task. */
    STMDB   R0!, {R4-R11}   
    
    /* Aggiorna il TCB del task attuale con il nuovo valore dello stack pointer */
    LDR     R1, =currentTCB /* R1 = Indirizzo della variabile currentTCB */
    LDR     R1, [R1]        /* R1 = Puntatore alla struct os_tcb_t */
    STR     R0, [R1]        /* TCB->stackPtr = R0 */

    /* 3. SCELTA DEL PROSSIMO TASK (LOGICA C) */
    PUSH    {LR}            /* Salva il Link Register (necessario per tornare dall'interrupt) */
    BL      OS_Scheduler    /* Chiama lo scheduler in C per aggiornare currentTCB */
    POP     {LR}            /* Ripristina il Link Register */

    /* 4. CARICAMENTO CONTESTO NUOVO TASK */
    LDR     R1, =currentTCB
    LDR     R1, [R1]        /* R1 punta ora al NUOVO TCB scelto dallo scheduler */
    LDR     R0, [R1]        /* R0 = nuovo TCB->stackPtr */

    /* LDMIA: Load Multiple Increment After.
       Ripristina i registri R4-R11 dallo stack del nuovo task. */
    LDMIA   R0!, {R4-R11}   
    
    /* Aggiorna il PSP (Process Stack Pointer) della CPU */
    MSR     PSP, R0         

    /* 5. RIABILITAZIONE INTERRUPT ED USCITA */
    CPSIE   I               
    BX      LR              /* Ritorno dall'eccezione: la CPU ripristina R0-R3, PC, xPSR */



.global OS_Launch_First_Task
.type OS_Launch_First_Task, %function

OS_Launch_First_Task:
    /* R0 contiene l'indirizzo di currentTCB */
    LDR     R0, [R0]        /* R0 = currentTCB (carica il puntatore alla struct) */
    LDR     R0, [R0]        /* R0 = currentTCB->stackPtr */

    /* 1. Ripristina il contesto software (R4-R11) */
    LDMIA   R0!, {R4-R11}

    /* 2. Imposta il PSP con l'indirizzo attuale dello stack */
    MSR     PSP, R0

    /* 3. Passa da MSP a PSP impostando il registro CONTROL */
    MOV     R1, #2          /* Usa R1 invece di R0 per non sporcare il puntatore stack */
    MSR     CONTROL, R1
    ISB                     

    /* 4. Caricamento Context Hardware senza conflitti */
    /* Invece di caricare R0-R3 con write-back su R0, carichiamo i valori separatamente */
    LDR     R1, [R0, #16]   /* Carica il valore del PC (offset 16 nel frame hardware) */
    LDR     R2, [R0, #20]   /* Carica il valore di xPSR (offset 20 nel frame hardware) */
    
    /* Aggiorniamo il PSP saltando tutto il frame hardware (8 registri * 4 byte = 32 byte) */
    ADD     R0, R0, #32     
    MSR     PSP, R0         /* Aggiorna il PSP finale */

    /* Riabilita gli interrupt prima di saltare */
    CPSIE   I               

    /* Salta al PC del task */
    BX      R1
