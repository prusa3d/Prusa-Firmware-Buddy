// button_draw.h
#ifndef _BUTTON_DRAW_H
#define _BUTTON_DRAW_H

#include "stdbool.h"
#include "guitypes.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void button_draw(rect_ui16_t rc_btn, const char *text, const font_t *pf, bool is_selected);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_BUTTON_DRAW_H
