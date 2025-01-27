/**
 * @file safety_timer_stubbed.hpp
 * @author Radek Vana
 * @brief Better safety timer
 * @date 2020-12-15
 */
#pragma once

#include "IPause.hpp"

class SafetyTimer {
    static constexpr millis_t default_interval = 30 * 60 * 1000;
    IPause *pBoundPause;
    millis_t interval; // zero if disabled
    millis_t last_reset;
    uint32_t knob_moves;
    uint32_t knob_clicks;
    SafetyTimer();
    SafetyTimer(const SafetyTimer &) = delete;

public:
    enum class expired_t : bool { no,
        yes };

    static SafetyTimer &Instance(); // Singleton
    expired_t Loop(); // Conditional reset of the internal counter of the safety timer
    void SetInterval(millis_t ms); // Set expire interval and reset the timer, 0 == disabled.

    void ReInit(); // set default values

    void BindPause(IPause &pause);
    void UnbindPause(IPause &pause);
};
