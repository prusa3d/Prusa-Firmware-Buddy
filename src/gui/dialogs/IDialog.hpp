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
    IDialog(rect_ui16_t rc = GuiDefaults.RectScreenBody);
    virtual ~IDialog();

    static constexpr rect_ui16_t get_radio_button_size(rect_ui16_t rc_btn) {
        rc_btn.y += (rc_btn.h - GuiDefaults.ButtonHeight - GuiDefaults.FrameWidth); // 30pixels for button (+ 10 space for grey frame)
        rc_btn.h = GuiDefaults.ButtonHeight;
        rc_btn.x += GuiDefaults.ButtonSpacing;
        rc_btn.w -= 2 * GuiDefaults.ButtonSpacing;
        return rc_btn;
    }

    void MakeBlocking() const; //could be static, but I want it to be usable only from dialog
};

void create_blocking_dialog_from_normal_window(window_t &dlg);
void create_blocking_dialog(IDialog &dlg);
