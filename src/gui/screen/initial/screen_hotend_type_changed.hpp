/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

/// Shown when boot-time hotend detection was inconclusive (a hot NTC and a cold-to-hot
/// PT1000 read alike on the nozzle ADC): asks the user to confirm the installed hotend
/// before running with a possibly-wrong temperature config.
class ScreenHotendTypeChanged final : public PseudoScreenCallback {
public:
    ScreenHotendTypeChanged();

    [[nodiscard]] static bool should_show();
};
