//ScreenHandler.hpp
#include "window_frame.hpp"
#pragma once

class Screens {

public:
    static Screens *Access() {
        static Screens ret;
        return &ret;
    }
    void Close() {}
    window_frame_t *Get() { return nullptr; };
};
