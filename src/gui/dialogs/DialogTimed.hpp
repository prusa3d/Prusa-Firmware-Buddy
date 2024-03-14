/**
 * @file DialogTimed.hpp
 * @author Radek Vana
 * @brief Base of dialog which opens after period without knob event and closes on knob event
 * @date 2021-03-05
 */
#pragma once

#include "IDialog.hpp"

class DialogTimed : public AddSuperWindow<IDialog> {
    const uint32_t open_period;
    uint32_t time_of_last_action;

    enum class DialogState {
        running = 0,
        paused = 1,
        disabled = 2,
    } state;

public:
    DialogTimed(window_t *parent, Rect16 rect, uint32_t open_period);

    void pauseDialog() {
        if (state != DialogState::disabled) {
            state = DialogState::paused;
        }
    }
    void resumeDialog() { state = DialogState::running; }
    void disableDialog() { state = DialogState::disabled; }
    bool isPaused() { return state == DialogState::paused; }
    bool isActive() { return state != DialogState::paused && state != DialogState::disabled; }

protected:
    enum class visibility_changed_t : bool { no,
        yes };

    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
    virtual void updateLoop(visibility_changed_t visibility_changed) = 0;
    bool isShowBlocked() const;
};
