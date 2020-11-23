//ScreenHandler.hpp
#include "window_frame.hpp"
#pragma once

class Screens {
    window_frame_t *frame;
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

    window_frame_t *Get() {
        return frame;
    };

    void Set(window_frame_t *current) {
        frame = current;

        window_t::ResetCapturedWindow();
        window_t::ResetFocusedWindow();

        if (!current->IsChildCaptured())
            current->SetCapture();
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

    void ResetTimeout() {}
};
