// st25dv64k.h

#pragma once

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void st25dv64k_init(void);

extern uint8_t st25dv64k_user_read(uint16_t address);

extern void st25dv64k_user_write(uint16_t address, uint8_t data);

extern void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size);

extern void st25dv64k_user_write_bytes(uint16_t address, void const *pdata, uint16_t size);

extern void st25dv64k_user_unverified_write_bytes(uint16_t address, void const *pdata, uint16_t size);

extern uint8_t st25dv64k_rd_cfg(uint16_t address);

extern void st25dv64k_wr_cfg(uint16_t address, uint8_t data);

extern void st25dv64k_present_pwd(uint8_t *pwd);

#ifdef __cplusplus
}
#endif
