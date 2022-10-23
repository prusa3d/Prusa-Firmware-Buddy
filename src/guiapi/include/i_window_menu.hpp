/**
 * @file i_window_menu.hpp
 * @brief abstract menu
 */

#pragma once

#include "window.hpp"
#include <stdint.h>

struct IWindowMenu : public AddSuperWindow<window_t> {
    IWindowMenu(window_t *parent, Rect16 rect)
        : AddSuperWindow<window_t>(parent, rect) { Enable(); }
};
