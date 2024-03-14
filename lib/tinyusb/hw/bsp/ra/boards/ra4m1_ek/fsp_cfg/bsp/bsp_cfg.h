/* generated configuration header file - do not edit */
#ifndef BSP_CFG_H_
#define BSP_CFG_H_

#include "bsp_clock_cfg.h"
#include "bsp_mcu_family_cfg.h"
#include "board_cfg.h"

#undef RA_NOT_DEFINED
#define BSP_CFG_RTOS (0)
#if defined(_RA_BOOT_IMAGE)
#define BSP_CFG_BOOT_IMAGE (1)
#endif
#define BSP_CFG_MCU_VCC_MV (3300)
#define BSP_CFG_STACK_MAIN_BYTES (0x800)
#define BSP_CFG_HEAP_BYTES (0x1000)
#define BSP_CFG_PARAM_CHECKING_ENABLE (1)
#define BSP_CFG_ASSERT (0)
#define BSP_CFG_ERROR_LOG (0)

#define BSP_CFG_PFS_PROTECT ((1))

#define BSP_CFG_C_RUNTIME_INIT ((1))
#define BSP_CFG_EARLY_INIT     ((0))

#define BSP_CFG_STARTUP_CLOCK_REG_NOT_RESET ((0))

#define BSP_CLOCK_CFG_MAIN_OSC_POPULATED (1)

#define BSP_CLOCK_CFG_MAIN_OSC_CLOCK_SOURCE	(0)
#define BSP_CLOCK_CFG_SUBCLOCK_DRIVE		(0)
#define BSP_CLOCK_CFG_SUBCLOCK_POPULATED	(1)
#define BSP_CLOCK_CFG_SUBCLOCK_STABILIZATION_MS 1000

#endif /* BSP_CFG_H_ */
