// screen_print_preview.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "window_roll_text.hpp"
#include "screen.hpp"
#include "display.h"
#include "gcode_info.hpp"
#include "gcode_description.hpp"
#include "fs_event_autolock.hpp"

class screen_print_preview_data_t : public AddSuperWindow<screen_t> {
    FS_EventAutolock lock_autoload_and_m600;

    window_roll_text_t title_text;
    window_icon_button_t print_button;
    window_text_t print_label;
    window_icon_button_t back_button;
    window_text_t back_label;
    WindowPreviewThumbnail thumbnail;
    GCodeInfo &gcode;

    GCodeInfoWithDescription gcode_description; //cannot be first

public:
    screen_print_preview_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    bool gcode_file_exists();
};
