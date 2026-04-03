.syntax unified
.cpu cortex-m33
.thumb

.global isr_pendsv
.type isr_pendsv, %function
.extern currentTCB
.extern OS_Scheduler

.section .text

/* isr_pendsv: Minimal context switch for MPU testing */
isr_pendsv:
    CPSID   I

    /* 1. Save outgoing context */
    MRS     R0, PSP
    CBZ     R0, skip_context_save

    TST     LR, #0x10
    IT      EQ
    VSTMDBEQ R0!, {S16-S31}

    STMDB   R0!, {R4-R11, R12, LR}

    LDR     R1, =currentTCB
    LDR     R1, [R1]
    STR     R0, [R1]

skip_context_save:
    /* 2. Call Scheduler */
    PUSH    {R4, LR}
    BL      OS_Scheduler
    
    /* 3. Configure MPU for the new task */
    LDR     R1, =currentTCB
    LDR     R1, [R1]        /* R1 = pointer to new TCB */
    LDR     R0, [R1, #16]   /* R0 = stack_base */
    LDR     R1, [R1, #20]   /* R1 = stack_size */
    BL      OS_MPU_Switch   /* Call C function */
    
    POP     {R4, LR}

    /* 4. Load incoming context */
    LDR     R1, =currentTCB
    LDR     R1, [R1]
    
    /* Set PSPLIM for hardware stack overflow protection */
    LDR     R2, [R1, #16]   /* R2 = stack_base */
    MSR     PSPLIM, R2
    
    LDR     R0, [R1]        /* R0 = new TCB->stackPtr */

    LDMIA   R0!, {R4-R11, R12, LR}

    TST     LR, #0x10
    IT      EQ
    VLDMIAEQ R0!, {S16-S31}

    MSR     PSP, R0

    /* 5. Force User Mode (Unprivileged) for the task */
    MOV     R1, #3
    MSR     CONTROL, R1
    ISB

    CPSIE   I
    BX      LR
