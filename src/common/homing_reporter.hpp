#pragma once

/**
 * @brief reports homing status
 * instance must be created at the beginning of G28 marlin gcode
 *
 * Usage:
 * It must be used within the same thread as Marlin (relying on idle loops)
 * 1 Call enable()
 * 2 Insert G28 gcode
 * 3 Periodically call consume_done()
 *
 */
class HomingReporter {
public:
    enum class State {
        disabled,
        enabled,
        in_progress,
        done
    };

    [[nodiscard]] HomingReporter() {
        if (state == State::enabled) {
            state = State::in_progress;
        }
    }

    ~HomingReporter() { state = State::done; }

    static void enable() { state = State::enabled; }
    static State consume_done() {
        State ret = state;
        if (state == State::done) {
            state = State::disabled;
        }
        return ret;
    }
    static bool block_red_screen() {
        return state == State::in_progress;
    }

private:
    static State state;
};
