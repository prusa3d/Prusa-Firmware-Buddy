#pragma once

#include "gui.hpp"
#include "screen_blue_error.hpp"

class ScreenWatchdog : public AddSuperWindow<ScreenBlueError> {
    char txt_err_title[39]; ///< Memory for title
    char txt_err_description[description_expected_chars]; ///< Memory for description

public:
    ScreenWatchdog();
};
