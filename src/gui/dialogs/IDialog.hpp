//IDialog.hpp
#pragma once

#include <stdint.h>
#include "window_frame.hpp"
#include "guitypes.hpp"
#include "GuiDefaults.hpp"
//interface for dialog
class IDialog : public window_frame_t {
    window_t *id_capture;

public:
    IDialog(Rect16 rc = GuiDefaults::RectScreenBody);
    virtual ~IDialog();

    static constexpr Rect16 get_radio_button_size(Rect16 rc_frame) {
        return Rect16(
            rc_frame.Left() + GuiDefaults::ButtonSpacing,
            rc_frame.Top() + (rc_frame.Height() - GuiDefaults::ButtonHeight - GuiDefaults::FrameWidth),
            rc_frame.Width() - 2 * GuiDefaults::ButtonSpacing,
            GuiDefaults::ButtonHeight);
    }

    void MakeBlocking(void (*action)() = []() {}) const; //could be static, but I want it to be usable only from dialog
};

void create_blocking_dialog_from_normal_window(window_t &dlg);
void create_blocking_dialog(IDialog &dlg);
