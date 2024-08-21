#include "touchscreen_common.hpp"
#include "touchscreen.hpp"
#include <option/has_side_leds.h>
#include <logging/log.hpp>

#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif

LOG_COMPONENT_DEF(Touch, logging::Severity::info);

METRIC_DEF(metric_touch_event_, "touch_evt", METRIC_VALUE_STRING, 0, METRIC_HANDLER_ENABLE_ALL);
METRIC_DEF(metric_touch_pos, "touch_pos", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);

metric_t *metric_touch_event() {
    return &metric_touch_event_;
}

Touchscreen_Base::LenientClickGuard::LenientClickGuard() {
    touchscreen.lenient_click_allowed_++;
}
Touchscreen_Base::LenientClickGuard::~LenientClickGuard() {
    touchscreen.lenient_click_allowed_--;
}

bool Touchscreen_Base::is_enabled() const {
    // !!! is_disabled_till_reset_ check must be before config_store
    return !is_disabled_till_reset_ && config_store().touch_enabled.get();
}

void Touchscreen_Base::set_enabled(bool set) {
    if (set == is_enabled()) {
        return;
    }

    config_store().touch_enabled.set(set);
    metric_record_string(metric_touch_event(), "set_enabled %i", set);
    log_info(Touch, "set enabled %i", set);
}

TouchscreenEvent Touchscreen_Base::get_event() {
    if (is_last_event_consumed_) {
        return TouchscreenEvent();
    }

#if HAS_SIDE_LEDS()
    // We cannot put this into recognize_gesture or such,
    // because those are in the timer interrupt and ActivityPing uses mutex.
    leds::side_strip_control.ActivityPing();
#endif

    is_last_event_consumed_ = true;
    return last_event_;
}

TouchscreenEvent Touchscreen_Base::get_last_event() const {
    return last_event_;
}

void Touchscreen_Base::update() {
    if (!is_hw_ok_) {
        return;
    }

    TouchState touch_state = last_touch_state_;
    touch_state.invalidate = false;
    update_impl(touch_state);

    if (touch_state.multitouch_point_count > 0) {
        metric_record_custom(&metric_touch_pos, " x=%i,y=%i", touch_state.multitouch_points[0].position.x, touch_state.multitouch_points[0].position.y);
    }

    // Gesture reported as invalid -> invalidate
    if (touch_state.invalidate) {
        gesture_recognition_state_ = GestureRecognitionState::invalid;
    }

    // Touch start -> set up gesture recognition
    else if (touch_state.multitouch_point_count == 1 && gesture_recognition_state_ == GestureRecognitionState::idle) {
        gesture_recognition_state_ = GestureRecognitionState::active;
        gesture_start_pos_ = touch_state.multitouch_points[0].position;
    }

    // Multiple touch points -> invalid gesture state
    else if (touch_state.multitouch_point_count > 1) {
        gesture_recognition_state_ = GestureRecognitionState::invalid;
    }

    // Touch end - recognize gesture
    else if (touch_state.multitouch_point_count == 0 && gesture_recognition_state_ != GestureRecognitionState::idle) {
        recognize_gesture();
        gesture_recognition_state_ = GestureRecognitionState::idle;
    }

    last_touch_state_ = touch_state;
}

void Touchscreen_Base::recognize_gesture() {
    if (gesture_recognition_state_ == GestureRecognitionState::invalid) {
        return;
    }

    assert(last_touch_state_.multitouch_point_count > 0);
    const point_ui16_t last_touch_pos = last_touch_state_.multitouch_points[0].position;

    const point_i16_t touch_pos_diff = point_i16_t::from_point(last_touch_pos) - point_i16_t::from_point(gesture_start_pos_);
    const point_i16_t touch_pos_diff_abs(abs(touch_pos_diff.x), abs(touch_pos_diff.y));

    /// Distance from the gesture_start_pos that is still considered a click
    const int16_t click_max_diff = lenient_click_allowed_ ? 3 : 0;

    /// Distance from the gesture_start_pos that starts being considered a swipe gesture
    static constexpr int16_t gesture_min_diff = 10;

    /// Tangens of max angle that considers as a swipe gesture
    static constexpr float swipe_max_angle_tan = 0.5;

    TouchscreenEvent event {
        .pos_x = gesture_start_pos_.x,
        .pos_y = gesture_start_pos_.y,
    };

    log_info(Touch, "abs diff %i %i", touch_pos_diff_abs.x, touch_pos_diff_abs.y);

    if (touch_pos_diff_abs.x <= click_max_diff && touch_pos_diff.y <= click_max_diff) {
        event.type = GUI_event_t::TOUCH_CLICK;

    } else if (touch_pos_diff_abs.y >= gesture_min_diff && static_cast<float>(touch_pos_diff_abs.x) / static_cast<float>(touch_pos_diff_abs.y) <= swipe_max_angle_tan) {
        event.type = touch_pos_diff.y < 0 ? GUI_event_t::TOUCH_SWIPE_UP : GUI_event_t::TOUCH_SWIPE_DOWN;

    } else if (touch_pos_diff_abs.x >= gesture_min_diff && static_cast<float>(touch_pos_diff_abs.y) / static_cast<float>(touch_pos_diff_abs.x) <= swipe_max_angle_tan) {
        event.type = touch_pos_diff.x < 0 ? GUI_event_t::TOUCH_SWIPE_LEFT : GUI_event_t::TOUCH_SWIPE_RIGHT;
    }

    metric_record_string(metric_touch_event(), "evt t%i x%i y%i", static_cast<int>(event.type), event.pos_x, event.pos_y);

    if (event) {
        last_event_ = event;
        is_last_event_consumed_ = false;
    }
}
