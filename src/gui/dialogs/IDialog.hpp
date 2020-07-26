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
};

void make_blocking_dialog(window_t &dlg);
