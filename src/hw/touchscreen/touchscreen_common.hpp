#pragma once

#include <atomic>

#include "metric.h"
#include "window_event.hpp"
#include "guitypes.hpp"
#include "display.h"

extern log_component_t LOG_COMPONENT(Touch) _LOG_COMPONENT_ATTRS;

/// Returns metric for logging touch events
metric_t *metric_touch_event();

struct TouchscreenEvent {

public:
    static_assert(static_cast<int>(GUI_event_t::_count) < (1 << 6));
    GUI_event_t type : 6 = GUI_event_t::_count;

    static_assert(display::GetW() <= (1 << 10));
    uint16_t pos_x : 10;

    static_assert(display::GetH() <= (1 << 10));
    uint16_t pos_y : 10;

public:
    inline explicit operator bool() const {
        return type != GUI_event_t::_count;
    }
};

// We gotta compress the event data a bit so that it fits into a 4-byte atomic.
// because we're passing it from an ISR
static_assert(sizeof(TouchscreenEvent) <= 4);

class Touchscreen_Base {

public:
    virtual ~Touchscreen_Base() = default;

public:
    bool is_enabled() const;

    virtual void set_enabled(bool set);

    inline bool is_hw_ok() const {
        return is_hw_ok_;
    }

    /// Marks the touchscreen as disabled till the printer is reset.
    /// This is used to disable touchscreen during BSODs, which might casue BSOD loop
    inline void disable_till_reset() {
        is_disabled_till_reset_ = true;
    }

public:
    /// Reads a touchscreen event (and cosumes it)
    TouchscreenEvent get_event();

    /// Returns last detected touchscreen event
    TouchscreenEvent get_last_event() const;

public:
    /// Polling/update event
    void update();

protected:
    struct TouchPointData {
        point_ui16_t position;
    };
    static constexpr size_t max_touch_points = 2;
    using TouchPointDataArray = std::array<TouchPointData, max_touch_points>;

    struct TouchState {
        TouchPointDataArray multitouch_points;
        size_t multitouch_point_count = 0;

        /// Marks that the touch has been deemed invalid and no events should be emitted
        /// This is used for discarding events caused by an EMC noise
        bool invalidate : 1 = false;
    };

protected:
    /// Updates the touch state variable based on current data from the touchscreen
    virtual void update_impl(TouchState &touch_state) = 0;

protected:
    bool is_hw_ok_ : 1 = false;

    bool is_disabled_till_reset_ : 1 = false;

private:
    void recognize_gesture();

private:
    TouchState last_touch_state_;
    std::atomic<TouchscreenEvent> last_event_;
    std::atomic<bool> is_last_event_consumed_ = true;

private:
    enum class GestureRecognitionState {
        idle, ///< No touch input currently
        active, ///< We're in a valid state, recognition is running
        invalid, ///< We've pressed multiple points, invalid gesture
    };
    GestureRecognitionState gesture_recognition_state_ = GestureRecognitionState::idle;
    point_ui16_t gesture_start_pos_;
};
