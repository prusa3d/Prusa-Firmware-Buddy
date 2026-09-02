#pragma once

// Cut-down stand-in for the manual stepping queue. It records every event
// instead of stepping, so the queue is empty at all times and full only once
// a test caps its capacity.

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

typedef uint16_t StepEventFlag_t;
enum StepEventFlag : StepEventFlag_t {
    STEP_EVENT_FLAG_STEP_X = 1 << 0,
    STEP_EVENT_FLAG_X_ACTIVE = 1 << 8,
    STEP_EVENT_FLAG_END_OF_MOTION = 1 << 13,
    STEP_EVENT_FLAG_KEEP_ALIVE = 1 << 14,
};
constexpr const uint16_t STEP_EVENT_FLAG_AXIS_MASK = 0x000Fu;

constexpr const int32_t STEP_TIMER_MAX_TICKS_LIMIT = std::numeric_limits<uint16_t>::max();

struct PreciseStepping {
    /// Set by a test to simulate a quick stop.
    static inline bool stop_pending = false;

    static bool stopping() { return stop_pending; }
    static void reset_from_halt(bool) {}
};

namespace precise_stepping::manual {

struct StubStepEvent {
    int delay_us;
    bool dir;
    StepEventFlag_t flags;
};

inline std::vector<StubStepEvent> stub_step_events;
inline size_t stub_queue_capacity = std::numeric_limits<size_t>::max();

inline bool is_full() { return stub_step_events.size() >= stub_queue_capacity; }

inline bool has_steps() { return false; }

inline void enqueue_step(int step_us, bool dir, StepEventFlag_t axis_flags) {
    stub_step_events.push_back({ step_us, dir, axis_flags });
}

/// How many events were queued when the motion was released, if it was.
inline std::optional<size_t> stub_released_after;

class HeldStart {
public:
    // Time starts at zero here.
    uint32_t release() {
        stub_released_after = stub_step_events.size();
        return 100;
    }
};

} // namespace precise_stepping::manual
