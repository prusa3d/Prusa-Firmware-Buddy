#pragma once
#include "precise_stepping.hpp"
#include <bsod/bsod.h>

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

} // namespace precise_stepping::manual
