#pragma once

#include "window_file_list.hpp"
#include "gcode_info.hpp"
#include <window_menu_adv.hpp>

class WindowFileBrowser : public window_file_list_t {
public:
    WindowFileBrowser(window_t *parent, Rect16 rect, const char *media_SFN_path);

    void SaveTopSFN();
    void clear_first_visible_sfn();

    int WriteNameToPrint(char *buff, size_t sz);

    union event_conversion_union {
        void *pvoid;
        enum class Action {
            file_selected,
            go_home,
        } action;
    };

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    /// Handles clicking on a focused item
    void handle_click();

    void go_up();
};
