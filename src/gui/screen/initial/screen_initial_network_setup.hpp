/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

class ScreenInitialNetworkSetup final : public PseudoScreenCallback {
public:
    ScreenInitialNetworkSetup();

    [[nodiscard]] static bool should_show();
};
