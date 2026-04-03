#include "os_mpu.h"
#include <stdio.h>

#define SCB_SHCSR           (*((volatile uint32_t *)0xE000ED24))
#define SCB_SHCSR_MEMFAULTENA (1UL << 16)

static void OS_MPU_Config_Attributes(void) {
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
    __asm volatile ("dsb");
    __asm volatile ("isb");

    /* 2. Configure MAIR */
    OS_MPU_Config_Attributes();
    SCB_SHCSR |= SCB_SHCSR_MEMFAULTENA;

    /* 3. Region 0: Flash (RO, User Access Allowed) */
    OS_MPU_Config_Region(REG_FLASH, 0x10000000, 0x10FFFFFF, 
                         MPU_RBAR_AP_RO_User | MPU_RBAR_XN_ALLOW, MPU_RLAR_ATTR_0);

    /* 4. Region 1: RAM Utente (I primi 512KB di SRAM) */
    /* Lasciamo gli ultimi 8KB (0x20080000-0x20081FFF) NON MAPPATI per il test Kamikaze */
    OS_MPU_Config_Region(REG_RAM, 0x20000000, 0x2007FFFF, 
                         MPU_RBAR_AP_RW_User | MPU_RBAR_XN_ALLOW, MPU_RLAR_ATTR_0);

    /* 5. Region 2: SIO (GPIO/LEDs - User Access Allowed) */
    OS_MPU_Config_Region(REG_SIO, 0xD0000000, 0xD000FFFF, 
                         MPU_RBAR_AP_RW_User | MPU_RBAR_XN_FORBID, MPU_RLAR_ATTR_1);

    /* 6. Region 3: UART (printf - User Access Allowed) */
    OS_MPU_Config_Region(REG_UART, 0x40034000, 0x40034FFF, 
                         MPU_RBAR_AP_RW_User | MPU_RBAR_XN_FORBID, MPU_RLAR_ATTR_1);

    /* 7. Enable MPU con PRIVDEFENA (Kernel vede tutto) */
    MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA | MPU_CTRL_HFNMIENA;
    __asm volatile ("dsb");
    __asm volatile ("isb");

    printf("[MPU] System Initialized (User Mode Ready)\n");
}

void OS_MPU_Switch(uint32_t base, uint32_t size) {
    (void)base;
    (void)size;
    /* La protezione dello Stack Isolation richiede un Linker Script separato per
       non sovrapporsi alla Region 1. Attualmente usiamo PSPLIM (Hardware) per lo Stack Overflow. */
}
