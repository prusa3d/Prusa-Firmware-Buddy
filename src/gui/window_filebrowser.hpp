#pragma once
#include "window_file_list.hpp"
#include "gcode_info.hpp"

class WindowFileBrowser : public AddSuperWindow<window_file_list_t> {
    static char root[FILE_PATH_BUFFER_LEN]; // we currently do not support multiple file browsers

public:
    WindowFileBrowser(window_t *parent, Rect16 rect, const char *media_SFN_path);

    static void SetRoot(const char *path);
    static void CopyRootTo(char *path);
    void SaveTopSFN();

    int WriteNameToPrint(char *buff, size_t sz);

    union event_conversion_union {
        void *pvoid;
        enum class Action {
            file_selected,
            go_home,
        } action;
    };

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    /// Handles clicking on a focused item
    void handle_click();

    void go_up();
};
