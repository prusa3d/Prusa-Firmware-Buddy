/**
 * @file safety_timer_stubbed.hpp
 * @author Radek Vana
 * @brief Better safety timer
 * @date 2020-12-15
 */
#pragma once

#include "IPause.hpp"
#include "../../lib/Marlin/Marlin/src/core/utility.h"

#include <option/has_human_interactions.h>

class SafetyTimer {
#if !HAS_HUMAN_INTERACTIONS()
    static constexpr millis_t default_interval = 10 * 60 * 1000;
#else
    static constexpr millis_t default_interval = 30 * 60 * 1000;
#endif
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
