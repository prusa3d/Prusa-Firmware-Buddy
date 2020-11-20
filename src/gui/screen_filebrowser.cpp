#include "screen_filebrowser.hpp"
#include "dbg.h"

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

#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "GuiDefaults.hpp"

#ifndef MAXPATHNAMELENGTH
    #define MAXPATHNAMELENGTH F_MAXPATHNAMELENGTH
#endif

#define LOG_ERROR(...) _dbg3("FILEBROWSER ERROR: " __VA_ARGS__)

// Default value could be rewritten by eeprom settings
WF_Sort_t screen_filebrowser_sort = WF_SORT_BY_TIME;

/// To save first/top visible item in the file browser
/// This is something else than the selected file for print
/// This is used to restore the content of the browser into previous state including the layout
constexpr unsigned int SFN_len = 13;
static char firstVisibleSFN[SFN_len] = "";

screen_filebrowser_data_t::screen_filebrowser_data_t()
    : AddSuperWindow<window_frame_t>()
    , header(this)
    , w_filelist(this, GuiDefaults::RectScreenBodyNoFoot) {
    screen_filebrowser_sort = (WF_Sort_t)variant_get_ui8(eeprom_get_var(EEVAR_FILE_SORT));

    header.SetIcon(IDR_PNG_folder_full_16px);
    static const char sf[] = N_("SELECT FILE");
    header.SetText(_(sf));

    // initialize the directory (and selected file) from marlin_vars
    marlin_vars_t *vars = marlin_vars();
    // here the strncpy is meant to be - need the rest of the buffer zeroed
    strncpy(w_filelist.sfn_path, vars->media_SFN_path, sizeof(w_filelist.sfn_path));
    // ensure null character at the end no matter what
    w_filelist.sfn_path[sizeof(w_filelist.sfn_path) - 1] = '\0';
    // cut by the filename to retain only the directory path
    char *c = strrchr(w_filelist.sfn_path, '/');
    *c = 0; // even if we didn't find the '/', c will point to valid memory
    // Moreover - the next characters after c contain the filename, which I want to start my cursor at!
    w_filelist.Load(screen_filebrowser_sort, c + 1, firstVisibleSFN);
    // SetItemIndex(1); // this is automatically done in the window file list
    w_filelist.SetCapture(); // hack to not change capture
}

static void screen_filebrowser_clear_firstVisibleSFN(marlin_vars_t *vars) {
    vars->media_SFN_path[0] = '/';
    vars->media_SFN_path[1] = 0;
    firstVisibleSFN[0] = 0; // clear the last top item
}

void screen_filebrowser_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    marlin_vars_t *vars = marlin_vars();
    if (event == GUI_event_t::MEDIA) {
        GuiMediaEventsHandler::state_t media_state = GuiMediaEventsHandler::state_t(int(param));
        if (media_state == GuiMediaEventsHandler::state_t::removed || media_state == GuiMediaEventsHandler::state_t::error) {
            screen_filebrowser_clear_firstVisibleSFN(vars);
            Screens::Access()->Close();
        }
    }

    if (event != GUI_event_t::CLICK) {
        SuperWindowEvent(sender, event, param);
        return;
    }

    static const char dirUp[] = "..";
    static const char slash = '/';

    bool currentIsFile;
    const char *currentSFN = w_filelist.CurrentSFN(&currentIsFile);

    if (!strcmp(currentSFN, dirUp) && window_file_list_t::IsPathRoot(w_filelist.sfn_path)) {
        screen_filebrowser_clear_firstVisibleSFN(vars);
        Screens::Access()->Close();
        return;
    }

    size_t sfnPathLen = strlen(w_filelist.sfn_path);
    if ((sfnPathLen + strlen(currentSFN) + 1) >= MAXPATHNAMELENGTH) {
        LOG_ERROR("path too long");
        SuperWindowEvent(sender, event, param);
        return;
    }
    if (!currentIsFile) {                // directory selected
        if (strcmp(currentSFN, dirUp)) { // not same -> not ..
            // append the dir name at the end of sfnPath
            if (w_filelist.sfn_path[sfnPathLen - 1] != slash) {
                w_filelist.sfn_path[sfnPathLen++] = slash;
            }
            strlcpy(w_filelist.sfn_path + sfnPathLen, currentSFN, FILE_PATH_MAX_LEN - sfnPathLen);
        } else {
            char *last = strrchr(w_filelist.sfn_path, slash);
            if (last == w_filelist.sfn_path) {
                // reached top level dir - ensure it only contains a slash
                w_filelist.sfn_path[0] = slash;
                w_filelist.sfn_path[1] = 0;
            } else {
                *last = '\0'; // truncate the string after the last "/"
            }
        }

        w_filelist.Load(screen_filebrowser_sort, nullptr, nullptr);

        // @@TODO we want to print the LFN of the dir name, which is very hard to do right now
        // However, the text is not visible on the screen yet...
        // header.SetText(strrchr(w_filelist.sfn_path, '/'));

    } else { // print the file
        if (vars->media_LFN && vars->media_SFN_path) {
            int written;
            if (window_file_list_t::IsPathRoot(w_filelist.sfn_path)) {
                written = snprintf(vars->media_SFN_path, FILE_PATH_MAX_LEN, "/%s", currentSFN);
            } else {
                written = snprintf(vars->media_SFN_path, FILE_PATH_MAX_LEN, "%s/%s", w_filelist.sfn_path, currentSFN);
            }
            if (written < 0 || written >= (int)FILE_PATH_MAX_LEN) {
                LOG_ERROR("failed to prepare file path for print");
                SuperWindowEvent(sender, event, param);
                return;
            }

            // displayed text - can be a 8.3 DOS name or a LFN
            strlcpy(vars->media_LFN, w_filelist.CurrentLFN(&currentIsFile), FILE_NAME_MAX_LEN);
            // save the top browser item
            strlcpy(firstVisibleSFN, w_filelist.TopItemSFN(), SFN_len);

            screen_print_preview_data_t::SetGcodeFilepath(vars->media_SFN_path);
            screen_print_preview_data_t::SetGcodeFilename(vars->media_LFN);
            Screens::Access()->Open(ScreenFactory::Screen<screen_print_preview_data_t>);

            return;
        }
    }
}
