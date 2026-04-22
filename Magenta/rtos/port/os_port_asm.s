.syntax unified
.cpu cortex-m33
.thumb

.global isr_pendsv
.type isr_pendsv, %function
.extern currentTCB
.extern OS_Scheduler
.extern OS_MPU_Switch

.global isr_svcall
.type isr_svcall, %function
.extern OS_Syscall_Handler

.section .text

/* isr_svcall: System Call Gatekeeper */
isr_svcall:
    /* 1. Determine which stack was in use (MSP or PSP) */
    TST     LR, #4
    ITE     EQ
    MRSEQ   R0, MSP
    MRSNE   R0, PSP
    
    /* 2. Extract SVC ID from [Stacked_PC - 2] into R1 */
    LDR     R1, [R0, #24]
    LDRB    R1, [R1, #-2]
    
    /* 3. Prepare arguments for C: R0 = ID, R1 = Pointer to Stack Frame */
    MOV     R2, R0          /* R2 = pointer */
    MOV     R0, R1          /* R0 = ID */
    MOV     R1, R2          /* R1 = pointer */
    
    /* 4. Call C Dispatcher */
    PUSH    {LR}
    BL      OS_Syscall_Handler
    POP     {LR}
    BX      LR

/* isr_pendsv: The working context switch (Clean version) */
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

    /* 5. Determine Privilege: Only force User Mode if flag is NOT set */
    LDR     R1, =currentTCB
    LDR     R1, [R1]
    LDRB    R2, [R1, #35]   /* Offset 35 = flags */
    TST     R2, #1          /* OS_TASK_FLAG_PRIVILEGED = 0x01 */
    BNE     skip_user_force

    /* Force User Mode (Unprivileged) for normal tasks */
    MOV     R1, #3
    MSR     CONTROL, R1
    ISB
    B       done_switch

skip_user_force:
    /* Ensure Privileged Mode with PSP */
    MOV     R1, #2
    MSR     CONTROL, R1
    ISB

done_switch:
    CPSIE   I
    BX      LR
