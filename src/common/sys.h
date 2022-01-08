// sys.h - system functions
#include "stdint.h"
#include "shared_config.h"

#pragma once

#ifdef __cplusplus

extern version_t &boot_version; // (address) from flash -> "volatile" is not necessary

extern "C" {
#endif //__cplusplus

extern volatile data_exchange_t ram_data_exchange;

extern void sys_reset(void) __attribute__((noreturn));

extern void sys_dfu_request_and_reset(void) __attribute__((noreturn));

extern bool sys_dfu_requested(void);

extern void sys_dfu_boot_enter(void) __attribute__((noreturn));

extern int sys_pll_is_enabled(void);

extern void sys_pll_disable(void);

extern void sys_pll_enable(void);

extern int sys_sscg_is_enabled(void);

extern void sys_sscg_disable(void);

extern void sys_sscg_enable(void);

extern void sys_sscg_set_config(int freq, int depth);

extern int sys_sscg_get_config(float *pfreq, float *pdepth);

extern void sys_spi_set_prescaler(int prescaler_num);

extern int sys_fw_update_is_enabled(void);

extern void sys_fw_update_enable(void);

extern void sys_fw_update_disable(void);

extern int sys_fw_update_on_restart_is_enabled(void);

extern void sys_fw_update_on_restart_enable(void);

extern void sys_fw_update_older_on_restart_enable(void);

extern void sys_fw_update_on_restart_disable(void);

extern int sys_fw_is_valid(void);

extern int sys_fw_invalidate(void);

extern int sys_fw_validate(void);

extern int sys_flash_is_empty(void *ptr, int size);

extern int sys_flash_write(void *dst, void *src, int size);

extern int sys_flash_erase_sector(unsigned int sector);

#ifdef __cplusplus
}
#endif //__cplusplus
