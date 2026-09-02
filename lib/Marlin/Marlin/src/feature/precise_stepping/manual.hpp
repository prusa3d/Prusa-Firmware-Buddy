#pragma once
#include "precise_stepping.hpp"
#include <bsod/bsod.h>
#include <optional>

// A few handy function when one wants to bypass whole Planner and generate
// steps manually (e.g., precise calibration motions such as pressure advance
// or input shaping calibration).

namespace precise_stepping::manual {

inline bool is_full() {
    buddy::InterruptDisabler _;
    return PreciseStepping::is_step_event_queue_full();
}

inline bool has_steps() {
    buddy::InterruptDisabler _;
    return PreciseStepping::has_step_events_queued();
}

inline void enqueue_step(int step_us, bool dir, StepEventFlag_t axis_flags) {
    debug_assert(step_us >= 0);
    debug_assert(step_us <= STEP_TIMER_MAX_TICKS_LIMIT);
    uint16_t next_queue_head = 0;

    buddy::InterruptDisabler _;
    step_event_u16_t *step_event = PreciseStepping::get_next_free_step_event(next_queue_head);
    step_event->time_ticks = step_us;
    step_event->flags = axis_flags;
    if (dir) {
        step_event->flags ^= STEP_EVENT_FLAG_DIR_MASK;
    }
    PreciseStepping::step_event_queue.head = next_queue_head;
}

/// Holds the step ISR off from an empty queue, so that the queue can be filled
/// ahead and then started at a known moment. Left alone, the ISR would run the
/// first event at its next idle poll, up to a millisecond away, and an event
/// queued after it was due would run late, shifting the rest of the motion.
class HeldStart {
public:
    HeldStart() {
        debug_assert(!has_steps());
    }

    // The compare left from before the hold is long past.
    ~HeldStart() {
        if (guard) {
            release();
        }
    }

    HeldStart(const HeldStart &) = delete;
    HeldStart &operator=(const HeldStart &) = delete;

    /// Lets the ISR run the queue `delay_us` from now, which is just enough
    /// to leave this function. Returns that moment in ticks_us(); the first
    /// event runs then, whatever its delay. Releases on destruction if not
    /// called.
    uint32_t release() {
        constexpr uint16_t delay_us = 100;
        const auto timer = &TimerHandle[STEP_TIMER_NUM].handle;

        buddy::InterruptDisabler _;
        // Read at once: from here on, ticks_us() misses a second rolling over.
        const uint32_t now_us = ticks_us();
        const uint16_t now_ticks = __HAL_TIM_GET_COUNTER(timer);

        __HAL_TIM_SET_COMPARE(timer, TIM_CHANNEL_1, static_cast<uint16_t>(now_ticks + delay_us));
        // Matches while held would run the ISR at once.
        __HAL_TIM_CLEAR_IT(timer, TIM_IT_CC1);
        NVIC_ClearPendingIRQ(TIM8_BRK_TIM12_IRQn);
        PreciseStepping::left_ticks_to_next_step_event = 0;
        PreciseStepping::last_step_isr_delay = 0;
        guard.reset();
        return now_us + delay_us;
    }

private:
    std::optional<StepIsrDisabler> guard { std::in_place };
};

} // namespace precise_stepping::manual
