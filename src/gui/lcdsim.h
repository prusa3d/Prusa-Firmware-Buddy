// lcdsim.h
#ifndef _LCDSIM_H
#define _LCDSIM_H

#include <inttypes.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifdef LCDSIM
extern uint32_t lcdsim_inval_mask[LCDSIM_ROWS];
#endif

extern void lcdsim_init(void);

extern void lcdsim_invalidate(void);

extern uint8_t lcdsim_char_at(uint8_t col, uint8_t row);

extern uint8_t *lcdsim_user_charset_ptr(void);

extern uint16_t lcdsim_grab_text(char *text);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _LCDSIM_H
