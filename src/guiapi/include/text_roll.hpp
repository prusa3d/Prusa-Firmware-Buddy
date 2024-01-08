// text_roll.hpp
#pragma once

#include "display_helper.h"

enum class invalidate_t { no,
    yes };

class txtroll_t {
    static constexpr uint32_t base_tick_ms = 40;
    static constexpr uint32_t wait_before_roll_ms = 300;
    static constexpr uint32_t wait_after_roll_ms = 1000;

    enum class phase_t : uint8_t {
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

    static Rect16 rect_meas(Rect16 rc, string_view_utf8 text, Font font, padding_ui8_t padding, Align_t alignment);
    static uint16_t meas(Rect16 rc, string_view_utf8 text, Font pf);

    void renderTextAlign(Rect16 rc, string_view_utf8 text, Font font, color_t clr_back, color_t clr_text, padding_ui8_t padding, Align_t alignment, bool fill_rect) const;

public:
    txtroll_t()
        // rect has default ctor
        : phase_progress(0)
        , draw_progress(0)
        , count_from_init(0)
        , count(0)
        , phase(phase_t::uninitialized)
        , px_cd(0)
        , font_w(0) { ++instance_counter; }

    ~txtroll_t() { --instance_counter; }

    void Init(Rect16 rc, string_view_utf8 text, Font font, padding_ui8_t padding, Align_t alignment);
    invalidate_t Tick();
    void RenderTextAlign(Rect16 rc, string_view_utf8 text, Font font, color_t clr_back, color_t clr_text, padding_ui8_t padding, Align_t alignment, bool fill_rect = true) const;
    bool NeedInit() const { return phase == phase_t::uninitialized; }
    void Reset() {
        if (phase != phase_t::uninitialized) {
            phase = phase_t::init_roll;
        }
    }
    void Deinit() { phase = phase_t::uninitialized; }
    void Stop() {
        if (phase != phase_t::uninitialized) {
            phase = phase_t::idle;
        }
    }
    void Pause() {
        if (phase != phase_t::uninitialized) {
            phase = phase_t::paused;
        }
    }
    static bool HasInstance() { return instance_counter != 0; }
    static uint32_t GetBaseTick() { return base_tick_ms; }
};
