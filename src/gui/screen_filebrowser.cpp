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
#include <logging/log.hpp>
#include "img_resources.hpp"
#include <guiconfig/GuiDefaults.hpp>

#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"

LOG_COMPONENT_REF(GUI);

screen_filebrowser_data_t::screen_filebrowser_data_t()
    : screen_t()
    , header(this)
    , file_browser(this, GuiDefaults::RectScreenNoHeader, GCodeInfo::getInstance().GetGcodeFilepath()) {
    header.SetIcon(&img::folder_full_16x16);
    header.SetText(_("PROJECTS"));

    CaptureNormalWindow(file_browser);
}

void screen_filebrowser_data_t::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
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
        browser().clear_first_visible_sfn();
        Screens::Access()->Get()->Validate(); // Do not redraw this
        Screens::Access()->Close();
    }
}

void screen_filebrowser_data_t::printTheFile() {
    // save the top browser item
    browser().SaveTopSFN();

    std::array<char, FILE_PATH_BUFFER_LEN> path;
    const auto written = browser().WriteNameToPrint(path.data(), path.size());
    if (written < 0 || static_cast<size_t>(written) >= path.size()) {
        log_error(GUI, "Failed to prepare file path for print");
        return;
    }
    print_begin(path.data());
}

void screen_filebrowser_data_t::goHome() {
    browser().clear_first_visible_sfn();
    Screens::Access()->Close();
}
