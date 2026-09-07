/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

class ScreenTouchDriverFailed final : public PseudoScreenCallback {
public:
    ScreenTouchDriverFailed();

    [[nodiscard]] static bool should_show();
};
