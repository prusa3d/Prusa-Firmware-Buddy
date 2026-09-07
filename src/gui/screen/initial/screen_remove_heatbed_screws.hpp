/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

class ScreenRemoveHeatbedScrews final : public PseudoScreenCallback {
public:
    ScreenRemoveHeatbedScrews();

    [[nodiscard]] static bool should_show();
};
