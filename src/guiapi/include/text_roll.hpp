//text_roll.hpp
#pragma once

#include "display_helper.h"

static const uint8_t TEXT_ROLL_DELAY_MS = 20; //todo i think system cannot shoot events this fast
static const uint16_t TEXT_ROLL_INITIAL_DELAY_MS = 1000;
static const uint8_t TXTROLL_SETUP_INIT = 0;
static const uint8_t TXTROLL_SETUP_DONE = 1;
static const uint8_t TXTROLL_SETUP_IDLE = 2;

typedef enum {
    ROLL_SETUP = 0,
    ROLL_GO,
    ROLL_STOP,
    ROLL_RESTART,
} TXTROLL_PHASE_t;

class txtroll_t {

    //static size_t instance_counter;

    static Rect16 rect_meas(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint16_t flags);
    static uint16_t meas(Rect16 rc, string_view_utf8 text, const font_t *pf);

public:
    Rect16 rect;
    uint16_t progress;
    uint16_t count;
    uint8_t phase;
    uint8_t setup;
    uint8_t px_cd;
    constexpr txtroll_t()
        //rect has default ctor
        : progress(0)
        , count(0)
        , phase(0)
        , setup(0)
        , px_cd(0) {}

    void Init(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint8_t alignment);
    void Phasing(window_t *pWin, font_t *font);
};

extern void render_roll_text_align(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint8_t alignment, color_t clr_back, color_t clr_text, const txtroll_t *roll);
