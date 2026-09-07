/// @file
#pragma once

#include <gui/pseudo_screen_callback.hpp>

/// Welcomes the user and offers to guide them through the setup process.
class ScreenWelcome final : public PseudoScreenCallback {
public:
    ScreenWelcome();
};
