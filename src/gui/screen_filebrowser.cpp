#include "screen_filebrowser.hpp"

#include "config.h"
#include "stdlib.h"
#include "usb_host.h"
#include "cmsis_os.h"
#include "marlin_client.h"
#include "screen_print_preview.hpp"
#include "print_utils.hpp"
#include "ScreenHandler.hpp"
#include "eeprom.h"
#include "i18n.h"
#include "gui_media_events.hpp"
#include "log.h"
#include "png_resources.hpp"
#include "GuiDefaults.hpp"

#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"

LOG_COMPONENT_REF(GUI);

screen_filebrowser_data_t::screen_filebrowser_data_t()
    : AddSuperWindow<screen_t>()
    , header(this)
    , file_browser(this, GuiDefaults::RectScreenNoHeader, marlin_vars()->media_SFN_path) {

    header.SetIcon(&png::folder_full_16x16);
    static const char sf[] = N_("PROJECTS");
    header.SetText(_(sf));

    CaptureNormalWindow(file_browser);
}

void screen_filebrowser_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::MEDIA) {
        MediaState_t media_state = MediaState_t(int(param));
        if (media_state == MediaState_t::removed || media_state == MediaState_t::error) {
            clearFirstVisibleSFN();
            Screens::Access()->Close();
        }
    }

    switch (event) {
    case GUI_event_t::CHILD_CLICK: {
        WindowFileBrowser::event_conversion_union un;
        un.pvoid = param;
        switch (un.action) {
        case WindowFileBrowser::event_conversion_union::Action::GoHome:
            goHome();
            return;
        case WindowFileBrowser::event_conversion_union::Action::FileSelected:
            printTheFile();
            return;
        }
    } break;
    default:
        break;
    }
}

void screen_filebrowser_data_t::clearFirstVisibleSFN() {
    FileBrowser::CopyRootTo(marlin_vars()->media_SFN_path);
}

/**
 * @brief write to (vars->media_LFN && vars->media_SFN_path to start print
 * super unsafe code
 * it is being done without any lock,
 *  - pointer can change during write (potential to write to deleted memory)
 *  - or multiple threads can write it at the same time
 *  - even read while another thread is writing can be corrupted
 */
void screen_filebrowser_data_t::printTheFile() {
    marlin_vars_t *vars = marlin_vars();
    if (vars->media_LFN && vars->media_SFN_path) {
        int written;
        //@@TODO:check for "/" on last place of path and if yes do not add "/"
        written = file_browser.WriteNameToPrint(vars->media_SFN_path, FILE_PATH_BUFFER_LEN);
        if (written < 0 || written >= (int)FILE_PATH_BUFFER_LEN) {
            log_error(GUI, "Failed to prepare file path for print");
            return;
        }

        // displayed text - can be a 8.3 DOS name or a LFN
        strlcpy(vars->media_LFN, file_browser.CurrentLFN(), FILE_NAME_BUFFER_LEN);
        // save the top browser item
        file_browser.SaveTopSFN();

        print_begin(vars->media_SFN_path, false);

        return;
    }
}

void screen_filebrowser_data_t::goHome() {
    clearFirstVisibleSFN();
    Screens::Access()->Close();
}
