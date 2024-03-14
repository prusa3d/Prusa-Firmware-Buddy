#pragma once
#include "gui.hpp"
#include "window_menu_adv.hpp"
#include "window_header.hpp"
#include "screen.hpp"
#include "window_dlg_wait.hpp"

class screen_filebrowser_data_t : public AddSuperWindow<screen_t> {
    window_header_t header;
    FileBrowser file_browser;

    static void clearFirstVisibleSFN(); // this method writes into pointer received from marlin_vars, it is super ugly
    void printTheFile();
    void goHome();

    /**
     * @brief Check if media is removed and close filebrowser.
     * @param media_state media state from event or from global state
     */
    void checkMissingMedia(MediaState_t media_state);

public:
    screen_filebrowser_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
