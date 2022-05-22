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

#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "GuiDefaults.hpp"

#ifndef MAXPATHNAMELENGTH
    #define MAXPATHNAMELENGTH F_MAXPATHNAMELENGTH
#endif

// Default value could be rewritten by eeprom settings
WF_Sort_t screen_filebrowser_sort = WF_SORT_BY_TIME;

/// To save first/top visible item in the file browser
/// This is something else than the selected file for print
/// This is used to restore the content of the browser into previous state including the layout
constexpr unsigned int SFN_len = 13;
static char firstVisibleSFN[SFN_len];
char screen_filebrowser_data_t::root[FILE_PATH_BUFFER_LEN] = "/usb";

screen_filebrowser_data_t::screen_filebrowser_data_t()
    : AddSuperWindow<screen_t>()
    , header(this)
    , w_filelist(this,
          GuiDefaults::ScreenWidth < 480 ? GuiDefaults::RectScreenNoHeader.TopLeft() : point_i16_t({ int16_t(16), int16_t(GuiDefaults::HeaderHeight + 5) }),
          GuiDefaults::ScreenWidth < 480 ? display::GetW() : display::GetW() - 40)
    , gcode_info(GCodeInfo::getInstance()) {
    screen_filebrowser_sort = (WF_Sort_t)eeprom_get_ui8(EEVAR_FILE_SORT);

    header.SetIcon(IDR_PNG_folder_full_16px);
    static const char sf[] = N_("PROJECTS");
    header.SetText(_(sf));

    //set root of the file list
    window_file_list_t::SetRoot(root);
    // initialize the directory (and selected file) from marlin_vars
    marlin_vars_t *vars = marlin_vars();
    // here the strncpy is meant to be - need the rest of the buffer zeroed
    strncpy(w_filelist.sfn_path, vars->media_SFN_path, sizeof(w_filelist.sfn_path));
    // ensure null character at the end no matter what
    w_filelist.sfn_path[sizeof(w_filelist.sfn_path) - 1] = '\0';
    // cut by the filename to retain only the directory path
    char *c = strrchr(w_filelist.sfn_path, '/');
    *c = 0; // even if we didn't find the '/', c will point to valid memory
    //check if we are at least in the root directory, if not move to root directory
    if (strstr(w_filelist.sfn_path, root) != w_filelist.sfn_path) {
        strlcpy(w_filelist.sfn_path, root, FILE_PATH_BUFFER_LEN);
    }
    // Moreover - the next characters after c contain the filename, which I want to start my cursor at!
    w_filelist.Load(screen_filebrowser_sort, c + 1, firstVisibleSFN);
    // SetItemIndex(1); // this is automatically done in the window file list
    CaptureNormalWindow(w_filelist);
}

void screen_filebrowser_data_t::clear_firstVisibleSFN(marlin_vars_t *vars) {
    strlcpy(vars->media_SFN_path, root, FILE_PATH_BUFFER_LEN);
}

void screen_filebrowser_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    marlin_vars_t *vars = marlin_vars();
    if (event == GUI_event_t::MEDIA) {
        MediaState_t media_state = MediaState_t(int(param));
        if (media_state == MediaState_t::removed || media_state == MediaState_t::error) {
            clear_firstVisibleSFN(vars);
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
        clear_firstVisibleSFN(vars);
        Screens::Access()->Close();
        return;
    }

    size_t sfnPathLen = strlen(w_filelist.sfn_path);
    if ((sfnPathLen + strlen(currentSFN) + 1) >= MAXPATHNAMELENGTH) {
        log_error(GUI, "Path too long");
        SuperWindowEvent(sender, event, param);
        return;
    }
    if (!currentIsFile) {                // directory selected
        if (strcmp(currentSFN, dirUp)) { // not same -> not ..
            // append the dir name at the end of sfnPath
            if (w_filelist.sfn_path[sfnPathLen - 1] != slash) {
                w_filelist.sfn_path[sfnPathLen++] = slash;
            }
            strlcpy(w_filelist.sfn_path + sfnPathLen, currentSFN, FILE_PATH_BUFFER_LEN - sfnPathLen);
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
            //@@TODO:check for "/" on last place of path and if yes do not add "/"
            written = snprintf(vars->media_SFN_path, FILE_PATH_BUFFER_LEN, "%s/%s", w_filelist.sfn_path, currentSFN);
            if (written < 0 || written >= (int)FILE_PATH_BUFFER_LEN) {
                log_error(GUI, "Failed to prepare file path for print");
                SuperWindowEvent(sender, event, param);
                return;
            }

            // displayed text - can be a 8.3 DOS name or a LFN
            strlcpy(vars->media_LFN, w_filelist.CurrentLFN(&currentIsFile), FILE_NAME_BUFFER_LEN);
            // save the top browser item
            strlcpy(firstVisibleSFN, w_filelist.TopItemSFN(), SFN_len);

            gcode_info.SetGcodeFilepath(vars->media_SFN_path);
            gcode_info.SetGcodeFilename(vars->media_LFN);
            Screens::Access()->Open(ScreenFactory::Screen<screen_print_preview_data_t>);

            return;
        }
    }
}
void screen_filebrowser_data_t::SetRoot(const char *path) {
    strlcpy(root, path, FILE_PATH_BUFFER_LEN);
}
