// ScreenHandler.hpp
#include "screen.hpp"
#pragma once

class Screens {
    screen_t *frame;
    Screens()
        : frame(nullptr) {
    }

public:
    static Screens *Access() {
        static Screens ret;
        return &ret;
    }
    void Close() {}

    bool ConsumeClose() {
        return false;
    }

    screen_t *Get() const {
        return frame;
    };

    void Set(screen_t *current) {
        frame = current;

        window_t::ResetFocusedWindow();

        /// need to be reset also focused ptr
        if (!current->IsFocused() && !current->IsChildFocused()) {
            window_t *child = current->GetFirstEnabledSubWin();
            if (child) {
                child->SetFocus();
            } else {
                current->SetFocus();
            }
        }
    };

    void ScreenEvent(window_t *sender, GUI_event_t event, void *param) {}

    void WindowEvent(GUI_event_t event, void *param) {}

    void ResetTimeout() {}
};
