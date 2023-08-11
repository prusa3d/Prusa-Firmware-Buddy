#pragma once

#include "gui.hpp"
#include "screen_blue_error.hpp"

class ScreenHardfault : public AddSuperWindow<ScreenBlueError> {
    char txt_err_description[description_expected_chars]; ///< Memory for description

public:
    ScreenHardfault();
};
