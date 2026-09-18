/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

/// Asks the user to confirm the installed hotend when boot detection could not decide it.
class ScreenHotendTypeChanged final : public PseudoScreenCallback {
public:
    ScreenHotendTypeChanged();

    [[nodiscard]] static bool should_show();
};
