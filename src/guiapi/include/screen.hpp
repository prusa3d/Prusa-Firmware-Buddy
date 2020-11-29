/**
 * @file screen.hpp
 * @author Radek Vana
 * @brief Similar to frame but can have dialogs.
 * @date 2020-11-29
 */

#pragma once

#include "window_frame.hpp"

class Screen : public AddSuperWindow<window_frame_t> {
    window_t *captured_normal_window; //might need to move it in window frame after menu refactoring

    window_t *first_dialog;
    window_t *last_dialog;

    window_t *first_strong_dialog;
    window_t *last_strong_dialog;

    window_t *first_popup;
    window_t *last_popup;

public:
    Screen();

protected:
};
