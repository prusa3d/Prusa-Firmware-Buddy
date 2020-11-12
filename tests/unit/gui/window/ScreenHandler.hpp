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
    bool ConsumeClose() { return false; }
    window_frame_t *Get() { return frame; };
    void Set(window_frame_t *fr) { frame = fr; };
};
