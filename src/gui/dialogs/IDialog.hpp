//IDialog.hpp
#pragma once

#include <stdint.h>
#include "window_frame.hpp"
#include "guitypes.h"

//interface for dialog
class IDialog : public window_frame_t {
    window_t *id_capture;

public:
    IDialog(rect_ui16_t rc = gui_defaults.scr_body_sz);
    virtual ~IDialog();

    static rect_ui16_t get_radio_button_size(rect_ui16_t rc_btn) {              // todo make constexpr
        rc_btn.y += (rc_btn.h - gui_defaults.btn_h - gui_defaults.frame_width); // 30pixels for button (+ 10 space for grey frame)
        rc_btn.h = gui_defaults.btn_h;
        rc_btn.x += gui_defaults.btn_spacing;
        rc_btn.w -= 2 * gui_defaults.btn_spacing;
        return rc_btn;
    }

    void MakeBlocking() const; //could be static, but I want it to be usable only from dialog
};

void create_blocking_dialog_from_normal_window(window_t &dlg);
void create_blocking_dialog(IDialog &dlg);
