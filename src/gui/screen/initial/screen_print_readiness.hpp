/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

class ScreenPrintReadiness final : public PseudoScreenCallback {
public:
    ScreenPrintReadiness();

    [[nodiscard]] static bool should_show();
};
