/**
 * @file
 * @brief st7789 implementation details, do not use outside st7789 implementation
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

//private flags (pin states)
#define FLG_CS  0x01 // current CS pin state
#define FLG_RS  0x02 // current RS pin state
#define FLG_RST 0x04 // current RST pin state

extern uint8_t st7789v_flg;

void st7789v_set_cs(void);
void st7789v_clr_cs(void);
void st7789v_set_rs(void);
void st7789v_clr_rs(void);
void st7789v_set_rst(void);
void st7789v_clr_rst(void);
void st7789v_reset(void);
void st7789v_delay_ms(uint32_t ms);

#ifdef __cplusplus
} //extern "C"
#endif
