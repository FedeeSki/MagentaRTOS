#ifndef OS_MPU_H
#define OS_MPU_H

#include <stdint.h>

/* MPU Registers (Cortex-M33) */
#define MPU_TYPE            (*((volatile uint32_t *)0xE000ED90))
#define MPU_CTRL            (*((volatile uint32_t *)0xE000ED94))
#define MPU_RNR             (*((volatile uint32_t *)0xE000ED98))
#define MPU_RBAR            (*((volatile uint32_t *)0xE000ED9C))
#define MPU_RLAR            (*((volatile uint32_t *)0xE000EDA0))
#define MPU_MAIR0           (*((volatile uint32_t *)0xE000EDC0))

/* MPU_CTRL Bits */
#define MPU_CTRL_ENABLE     (1UL << 0)
#define MPU_CTRL_HFNMIENA   (1UL << 1)
#define MPU_CTRL_PRIVDEFENA (1UL << 2)

/* MPU_RBAR (Base Address Register) */
#define MPU_RBAR_BASE_Msk   0xFFFFFFE0UL
#define MPU_RBAR_AP_RW_User (1UL << 1) /* Priv: RW, User: RW */
#define MPU_RBAR_AP_RO_User (3UL << 1) /* Priv: RO, User: RO */
#define MPU_RBAR_XN_ALLOW   (0UL << 0)
#define MPU_RBAR_XN_FORBID  (1UL << 0)

/* MPU_RLAR (Limit Address Register) */
#define MPU_RLAR_LIMIT_Msk  0xFFFFFFE0UL
#define MPU_RLAR_ATTR_0     (0UL << 1) /* Index in MAIR0 */
#define MPU_RLAR_ATTR_1     (1UL << 1)
#define MPU_RLAR_ENABLE     (1UL << 0)
/* Region IDs (Matching mpuARCH.txt + NS Alias) */
#define REG_FLASH_S         0
#define REG_SIO             1
#define REG_UART            2
#define REG_FLASH_NS        3
#define REG_STACK           4

/* MPU_RBAR Attributes */
#define MPU_RBAR_AP_PRW_URW (1UL << 1) /* Priv: RW, User: RW */
#define MPU_RBAR_AP_PRO_URO (3UL << 1) /* Priv: RO, User: RO */
#define MPU_RBAR_AP_PRW_UNO (0UL << 1) /* Priv: RW, User: None */

void OS_MPU_Init(void);
void OS_MPU_Switch(uint32_t stack_base, uint32_t stack_size);


#endif
