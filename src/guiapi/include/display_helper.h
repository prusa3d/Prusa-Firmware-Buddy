//helper.h
#ifndef _HELPER_H
#define _HELPER_H

#include "guitypes.h"

#define RENDER_FLG_ALIGN 0x00ff       // alignment mask (ALIGN_xxx)
#define RENDER_FLG_ROPFN 0x0f00       // raster operation function mask (ROPFN_xxx << 8)
#define RENDER_FLG_WORDB 0x1000       // multiline text
#define RENDER_FLG(a, r) (a | r << 8) // render flag macro (ALIGN and ROPFN)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void render_text_align(rect_ui16_t rc, const char *text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags);

extern void render_icon_align(rect_ui16_t rc, uint16_t id_res, color_t clr0, uint16_t flags);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_HELPER_H
