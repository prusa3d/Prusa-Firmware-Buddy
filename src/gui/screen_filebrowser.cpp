#include "screen_filebrowser.hpp"

#include "config.h"
#include "stdlib.h"
#include "usb_host.h"
#include "cmsis_os.h"
#include "marlin_client.hpp"
#include "screen_print_preview.hpp"
#include "print_utils.hpp"
#include "ScreenHandler.hpp"
#include "i18n.h"
#include "gui_media_events.hpp"
#include "log.h"
#include "img_resources.hpp"
#include "GuiDefaults.hpp"

#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"

LOG_COMPONENT_REF(GUI);

screen_filebrowser_data_t::screen_filebrowser_data_t()
    : AddSuperWindow<screen_t>()
    , header(this)
    , file_browser(this, GuiDefaults::RectScreenNoHeader, gui_media_SFN_path) {
    header.SetIcon(&img::folder_full_16x16);
    static const char sf[] = N_("PROJECTS");
    header.SetText(_(sf));

    CaptureNormalWindow(file_browser);
}

void screen_filebrowser_data_t::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::MEDIA:
        checkMissingMedia(MediaState_t(reinterpret_cast<int>(param)));
        break;
    case GUI_event_t::LOOP: // If media is removed during preprint, the even won't come
        checkMissingMedia(GuiMediaEventsHandler::Get());
        break;
    case GUI_event_t::CHILD_CLICK: {
        WindowFileBrowser::event_conversion_union un;
        un.pvoid = param;
        switch (un.action) {
        case WindowFileBrowser::event_conversion_union::Action::go_home:
            goHome();
            return;
        case WindowFileBrowser::event_conversion_union::Action::file_selected:
            printTheFile();
            return;
        }
    } break;
    default:
        break;
    }
}

void screen_filebrowser_data_t::checkMissingMedia(MediaState_t media_state) {
    if (media_state == MediaState_t::removed || media_state == MediaState_t::error) {
        clearFirstVisibleSFN();
        Screens::Access()->Get()->Validate(); // Do not redraw this
        Screens::Access()->Close();
    }
}

void screen_filebrowser_data_t::clearFirstVisibleSFN() {
    FileBrowser::CopyRootTo(gui_media_SFN_path);
}

void screen_filebrowser_data_t::printTheFile() {
    int written;
    //@@TODO:check for "/" on last place of path and if yes do not add "/"
    written = file_browser.WriteNameToPrint(gui_media_SFN_path, FILE_PATH_BUFFER_LEN);
    if (written < 0 || written >= (int)FILE_PATH_BUFFER_LEN) {
        log_error(GUI, "Failed to prepare file path for print");
        return;
    }

    // displayed text - can be a 8.3 DOS name or a LFN
    strlcpy(gui_media_LFN, file_browser.CurrentLFN(), FILE_NAME_BUFFER_LEN);
    // save the top browser item
    file_browser.SaveTopSFN();

    print_begin(gui_media_SFN_path);

    return;
}

void screen_filebrowser_data_t::goHome() {
    clearFirstVisibleSFN();
    Screens::Access()->Close();
}
