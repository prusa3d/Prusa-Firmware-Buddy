// IDialog.hpp
#pragma once

#include <stdint.h>
#include "screen_init_variant.hpp"
#include "window_frame.hpp"
#include "guitypes.hpp"
#include <guiconfig/GuiDefaults.hpp>

// interface for dialog
class IDialog : public AddSuperWindow<window_frame_t> {
    screen_init_variant underlying_screen_state;

public:
    IDialog(Rect16 rc = GuiDefaults::DialogFrameRect);
    IDialog(window_t *parent, Rect16 rc = GuiDefaults::DialogFrameRect);
    ~IDialog();
};
