// IDialog.hpp
#pragma once

#include <stdint.h>
#include "window_frame.hpp"
#include "guitypes.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include <functional>

// interface for dialog
class IDialog : public AddSuperWindow<window_frame_t> {
public:
    IDialog(Rect16 rc = GuiDefaults::DialogFrameRect);
    IDialog(window_t *parent, Rect16 rc = GuiDefaults::DialogFrameRect);
};
