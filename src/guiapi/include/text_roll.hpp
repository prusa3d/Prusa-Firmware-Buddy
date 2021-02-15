//text_roll.hpp
#pragma once

#include "display_helper.h"

enum class invalidate_t { no,
    yes };

class txtroll_t {
    enum {
        base_tick_ms = 40,
        wait_before_roll_ms = 2000,
        wait_after_roll_ms = 1000
    };

    enum class phase_t {
        uninitialized, // similar to idle, but init did not run
        init_roll,
        wait_before_roll,
        go,
        wait_after_roll,
        idle,
        paused,
    };

    Rect16 rect;
    uint16_t phase_progress;
    uint16_t draw_progress;
    uint16_t count_from_init;
    uint16_t count;
    phase_t phase;
    uint8_t px_cd;
    uint8_t font_w;

    static size_t instance_counter;

    static Rect16 rect_meas(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, Align_t alignment);
    static uint16_t meas(Rect16 rc, string_view_utf8 text, const font_t *pf);

    void renderTextAlign(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr_back, color_t clr_text, padding_ui8_t padding, Align_t alignment) const;

public:
    constexpr txtroll_t()
        //rect has default ctor
        : phase_progress(0)
        , draw_progress(0)
        , count_from_init(0)
        , count(0)
        , phase(phase_t::uninitialized)
        , px_cd(0)
        , font_w(0) { ++instance_counter; }

    ~txtroll_t() { --instance_counter; }

    void Init(Rect16 rc, string_view_utf8 text, const font_t *font, padding_ui8_t padding, Align_t alignment);
    invalidate_t Tick();
    void RenderTextAlign(Rect16 rc, string_view_utf8 text, const font_t *font, color_t clr_back, color_t clr_text, padding_ui8_t padding, Align_t alignment) const;
    bool NeedInit() const { return phase == phase_t::uninitialized; }
    void Reset() {
        if (phase != phase_t::uninitialized)
            phase = phase_t::init_roll;
    }
    void Deinit() { phase = phase_t::uninitialized; }
    void Stop() {
        if (phase != phase_t::uninitialized)
            phase = phase_t::idle;
    }
    void Pause() {
        if (phase != phase_t::uninitialized)
            phase = phase_t::paused;
    }
    static bool HasInstance() { return instance_counter != 0; }
    static uint32_t GetBaseTick() { return base_tick_ms; }
};
