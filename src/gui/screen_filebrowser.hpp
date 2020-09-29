#pragma once
#include "gui.hpp"
#include "window_file_list.hpp"
#include "window_header.hpp"

class screen_filebrowser_data_t : public AddSuperWindow<window_frame_t> {
    window_header_t header;
    window_file_list_t w_filelist;

public:
    screen_filebrowser_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
