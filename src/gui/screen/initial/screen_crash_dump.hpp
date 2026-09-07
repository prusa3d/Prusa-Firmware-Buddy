/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

class ScreenCrashDump final : public PseudoScreenCallback {
public:
    ScreenCrashDump();

    [[nodiscard]] static bool should_show();
};
