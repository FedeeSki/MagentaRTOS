#include "os_mpu.h"
#include <stdio.h>

#define SCB_SHCSR           (*((volatile uint32_t *)0xE000ED24))
#define SCB_SHCSR_MEMFAULTENA (1UL << 16)

static void OS_MPU_Config_Attributes(void) {
    /* MAIR0: Attr 0 = Normal, Attr 1 = Device */
    MPU_MAIR0 = (0xFFUL << 0) | (0x04UL << 8);
}

void OS_MPU_Config_Region(uint8_t region, uint32_t base, uint32_t limit, uint32_t rbar_attr, uint32_t rlar_attr) {
    MPU_RNR = region;
    MPU_RBAR = (base & MPU_RBAR_BASE_Msk) | rbar_attr;
    MPU_RLAR = (limit & MPU_RLAR_LIMIT_Msk) | rlar_attr | MPU_RLAR_ENABLE;
}

void OS_MPU_Init(void) {
    /* 1. Disable MPU */
    MPU_CTRL = 0;
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");

    /* 2. Configure MAIR */
    OS_MPU_Config_Attributes();
    SCB_SHCSR |= SCB_SHCSR_MEMFAULTENA;

    /* 3. Region 0: Flash Secure Alias (0x10... and 0x11...) */
    OS_MPU_Config_Region(REG_FLASH_S, 0x10000000, 0x11FFFFFF, 
                         MPU_RBAR_AP_RO_User | MPU_RBAR_XN_ALLOW, MPU_RLAR_ATTR_0);

    /* 4. Region 3: Flash Non-Secure Alias (0x00... and 0x01...) */
    OS_MPU_Config_Region(REG_FLASH_NS, 0x00000000, 0x01FFFFFF, 
                         MPU_RBAR_AP_RO_User | MPU_RBAR_XN_ALLOW, MPU_RLAR_ATTR_0);

    /* 5. Region 1: SIO (GPIO/LEDs Toggle) 
       Crucial for direct high-speed blink from tasks. */
    OS_MPU_Config_Region(REG_SIO, 0xD0000000, 0xD000FFFF, 
                         MPU_RBAR_AP_RW_User | MPU_RBAR_XN_FORBID, MPU_RLAR_ATTR_1);

    /* 6. Region 2: IO_BANK0 & UART0 (Pin Config and UART)
       Base: 0x40000000, Limit: 0x400FFFFF was too broad.
       We'll focus on the essential block for GPIO and UART: 0x40010000 to 0x4003FFFF. */
    OS_MPU_Config_Region(REG_UART, 0x40010000, 0x4003FFFF, 
                         MPU_RBAR_AP_RW_User | MPU_RBAR_XN_FORBID, MPU_RLAR_ATTR_1);

    /* 7. Enable MPU con PRIVDEFENA (Kernel sees global RAM at 0x20000000) */
    MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA | MPU_CTRL_HFNMIENA;
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");

    printf("[MPU] Correct Non-Overlapping Windows Active (User RAM Locked)\n");
}

/**
 * OS_MPU_Switch: Opens a dynamic window for the current task's stack.
 */
void OS_MPU_Switch(uint32_t stack_base, uint32_t stack_size) {
    uint32_t stack_limit = stack_base + (stack_size * 4) - 1;

    MPU_RNR = REG_STACK;
    MPU_RBAR = (stack_base & MPU_RBAR_BASE_Msk) | MPU_RBAR_AP_RW_User | MPU_RBAR_XN_FORBID;
    MPU_RLAR = (stack_limit & MPU_RLAR_LIMIT_Msk) | MPU_RLAR_ATTR_0 | MPU_RLAR_ENABLE;
    
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
}
