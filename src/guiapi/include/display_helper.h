//helper.h
#ifndef _HELPER_H
#define _HELPER_H

#include "guitypes.h"

#define RENDER_FLG_ALIGN           0x00ff       // alignment mask (ALIGN_xxx)
#define RENDER_FLG_ROPFN           0x0f00       // raster operation function mask (ROPFN_xxx << 8)
#define RENDER_FLG_WORDB           0x1000       // multiline text
#define RENDER_FLG(a, r)           (a | r << 8) // render flag macro (ALIGN and ROPFN)
#define TEXT_ROLL_DELAY_MS         50
#define TEXT_ROLL_INITIAL_DELAY_MS 4000
#define TXTROLL_SETUP_INIT         0
#define TXTROLL_SETUP_DONE         1
#define TXTROLL_SETUP_IDLE         2

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef enum {
    ROLL_SETUP = 0,
    ROLL_GO,
    ROLL_STOP,
    ROLL_RESTART,
} TXTROLL_PHASE_t;

typedef struct _txtroll_t {
    rect_ui16_t rect;
    uint16_t progress;
    uint16_t count;
    uint8_t phase;
    uint8_t setup;
    uint8_t px_cd;
} txtroll_t;

extern void render_text_align(rect_ui16_t rc, const char *text, const font_t *font, color_t clr0, color_t clr1, padding_ui8_t padding, uint16_t flags);

extern void render_icon_align(rect_ui16_t rc, uint16_t id_res, color_t clr0, uint16_t flags);

extern void roll_text_phasing(int16_t win_id, font_t *font, txtroll_t *roll);

extern void render_roll_text_align(rect_ui16_t rc, const char *text, font_t *font, padding_ui8_t padding, uint8_t alignment, color_t clr0, color_t clr1, txtroll_t *roll);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_HELPER_H
