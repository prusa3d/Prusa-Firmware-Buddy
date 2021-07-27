#pragma once
#include "gui.hpp"
#include "window_file_list.hpp"
#include "window_header.hpp"
#include "screen.hpp"
#include "gcode_info.hpp"

class screen_filebrowser_data_t : public AddSuperWindow<screen_t> {
    window_header_t header;
    window_file_list_t w_filelist;
    GCodeInfo &gcode_info;

public:
    screen_filebrowser_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
