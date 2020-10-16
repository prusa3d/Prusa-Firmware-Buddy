//text_roll.hpp
#pragma once

#include "display_helper.h"

static const uint8_t TEXT_ROLL_DELAY_MS = 20; //todo i think system cannot shoot events this fast
static const uint16_t TEXT_ROLL_INITIAL_DELAY_MS = 1000;

class txtroll_t {
    enum class phase_t {
        setup,
        go,
        stop,
        restart,
    };
    enum { BASE_TICK_MS = 20 };
    enum class setup_t {
        init,
        done,
        idle
    };
    Rect16 rect;
    uint16_t progress;
    uint16_t count;
    phase_t phase;
    setup_t setup;
    uint8_t px_cd;

    static size_t instance_counter;

    static Rect16 rect_meas(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint16_t flags);
    static uint16_t meas(Rect16 rc, string_view_utf8 text, const font_t *pf);

public:
    constexpr txtroll_t()
        //rect has default ctor
        : progress(0)
        , count(0)
        , phase(phase_t::setup)
        , setup(setup_t::init)
        , px_cd(0) { ++instance_counter; }

    ~txtroll_t() { --instance_counter; }

    void Init(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint8_t alignment);
    void Phasing(window_t *pWin, font_t *font);
    void RenderTextAlign(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, uint8_t alignment, color_t clr_back, color_t clr_text) const;
    bool NeedInit() const { return setup == setup_t::init; }
    bool IsSetupDone() const { return setup == setup_t::done; }
    void Reset(window_t *pWin);

    static bool HasInstance() { return instance_counter != 0; }
    static uint32_t GetBaseTick() { return BASE_TICK_MS; }
};
