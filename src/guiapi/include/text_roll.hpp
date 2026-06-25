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

    uint16_t phase_progress; //< phase countdown; used for waiting before and after roll
    uint16_t draw_progress; //< number of characters skipped due to roll
    uint16_t hidden_char_cnt;
    uint16_t count; //< countdown of number of characters to be hidden during rolling
    phase_t phase;
    uint8_t px_cd; //< pixel countdown; when it reaches 0, one character is removed and countdown is reset
    uint8_t font_w;

    static size_t instance_counter;

    static uint16_t meas(Rect16 rc, const string_view_utf8 &text, Font font, padding_ui8_t padding);

public:
    txtroll_t()
        // rect has default ctor
        : phase_progress(0)
        , draw_progress(0)
        , hidden_char_cnt(0)
        , count(0)
        , phase(phase_t::uninitialized)
        , px_cd(0)
        , font_w(0) { ++instance_counter; }

    ~txtroll_t() { --instance_counter; }

    void Init(const Rect16 &rect, const string_view_utf8 &text, Font font, padding_ui8_t padding);
    invalidate_t Tick();
    void render_text(const Rect16 &rect, const string_view_utf8 &text, Font font, Color clr_back, Color clr_text, padding_ui8_t padding = padding_ui8_t(0, 0, 0, 0), Align_t alignment = Align_t::Left()) const;
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
