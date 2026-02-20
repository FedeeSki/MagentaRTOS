.syntax unified
.cpu cortex-m33
.thumb

.global isr_pendsv
.type isr_pendsv, %function
.extern currentTCB
.extern OS_Scheduler

.section .text

/*
 * isr_pendsv: Context switch handler.
 */
isr_pendsv:
    /* 1. Disable interrupts for critical section */
    CPSID   I

    /* 2. Save the context of the outgoing task */
    MRS     R0, PSP         /* R0 = Process Stack Pointer of the current task */
    
    /* If PSP is 0, we are starting the first task, so skip saving context */
    CBZ     R0, skip_context_save

    /* Check if the FPU was used by this task. If so, save the FPU context. */
    TST     LR, #0x10       /* Test bit 4 of EXC_RETURN. If 0, FPU is active. */
    IT      EQ              /* If the result of TST is zero... */
    VSTMDBEQ R0!, {S16-S31}  /* Push the callee-saved FPU registers. */

    /* STMDB: Store Multiple Decrement Before.
       Saves registers R4-R11, R12, and LR onto the task's stack (10 words). */
    STMDB   R0!, {R4-R11, R12, LR}


    /* Update the TCB of the current task with the new stack pointer */
    LDR     R1, =currentTCB /* R1 = Address of the currentTCB variable */
    LDR     R1, [R1]        /* R1 = Pointer to the os_tcb_t struct */
    STR     R0, [R1]        /* TCB->stackPtr = R0 */

skip_context_save:
    /* 3. Choose the next task (C logic) */
    BL      OS_Scheduler    /* Call the C scheduler to update currentTCB */

    /* 4. Load the context of the new task */
    LDR     R1, =currentTCB
    LDR     R1, [R1]        /* R1 now points to the NEW TCB chosen by the scheduler */
    LDR     R0, [R1]        /* R0 = new TCB->stackPtr */

    /* LDMIA: Load Multiple Increment After.
       Restores registers R4-R11, R12, and LR from the new task's stack. */
    LDMIA   R0!, {R4-R11, R12, LR}

    /* Check if the incoming task uses the FPU. If so, restore the FPU context. */
    TST     LR, #0x10       /* Test bit 4 of EXC_RETURN */
    IT      EQ              /* If 0, FPU context needs to be restored. */
    VLDMIAEQ R0!, {S16-S31}  /* Pop the callee-saved FPU registers. */

    /* Update the CPU's PSP (Process Stack Pointer) */
    MSR     PSP, R0

    /* 5. Re-enable interrupts and exit */
    CPSIE   I
    BX      LR              /* Exception return: CPU restores R0-R3, PC, xPSR, and S0-S15 if used */

.global OS_Launch_First_Task
.type OS_Launch_First_Task, %function
/*
 * OS_Launch_First_Task: Starts the first task.
 */
OS_Launch_First_Task:
    /* R0 = Address of the currentTCB pointer */
    LDR R0, =currentTCB

    /* Load the stack pointer of the first task into R0 */
    LDR R1, [R0]        /* R1 = pointer to the first TCB */
    LDR R0, [R1]        /* R0 = stack pointer of the first TCB */

    /* 1. Restore the software context (R4-R11, R12, LR) */
    LDMIA   R0!, {R4-R11, R12, LR}

    /* 2. Check if the FPU was used by this task. If so, restore the FPU context. */
    TST     LR, #0x10       /* Test bit 4 of EXC_RETURN. If 0, FPU is active. */
    IT      EQ              /* If the result of TST is zero... */
    VLDMIAEQ R0!, {S16-S31} /* Pop the callee-saved FPU registers. */

    /* 3. Set the PSP to the task's stack pointer */
    MSR     PSP, R0

    /* 4. Switch from MSP to PSP by setting the CONTROL register */
    MOV     R1, #2
    MSR     CONTROL, R1
    ISB                 /* Instruction Synchronization Barrier */

    /* 5. "Return" to the task. */
    CPSIE   I
    BX      LR

