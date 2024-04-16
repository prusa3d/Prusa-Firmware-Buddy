// sys.h - system functions
#include "stdint.h"
#include "shared_config.h"

#pragma once

extern version_t &boot_version; // (address) from flash -> "volatile" is not necessary

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

extern int sys_fw_update_is_enabled(void);

extern void sys_fw_update_enable(void);

extern void sys_fw_update_disable(void);

extern int sys_flash_is_empty(void *ptr, int size);

extern int sys_flash_write(void *dst, void *src, int size);

extern int sys_flash_erase_sector(unsigned int sector);

/// @return true if version a < (major, minor, patch)
extern bool version_less_than(const version_t *a, const uint8_t major, const uint8_t minor, const uint8_t patch);
