/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

class ScreenEmergencyStopConsent final : public PseudoScreenCallback {
public:
    ScreenEmergencyStopConsent();

    [[nodiscard]] static bool should_show();
};
