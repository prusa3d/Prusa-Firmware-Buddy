#pragma once
#include "gui.hpp"
#include "window_file_list.hpp"
#include "window_header.hpp"
#include "screen.hpp"
#include "gcode_info.hpp"
#include "marlin_vars.h"

class screen_filebrowser_data_t : public AddSuperWindow<screen_t> {
    window_header_t header;
    window_file_list_t w_filelist;
    GCodeInfo &gcode_info;
    static void clear_firstVisibleSFN(marlin_vars_t *vars);
    static char root[FILE_PATH_BUFFER_LEN]; // we can have only one screen_filebrowser at time, so this variable can be static

public:
    screen_filebrowser_data_t();

    //sets root variable
    static void SetRoot(const char *path);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
