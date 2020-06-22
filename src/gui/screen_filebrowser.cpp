/*
 * screen_menu_filebrowser.cpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#include "gui.h"
#include "dbg.h"
#include "window_file_list.h"
#include "window_header.h"
#include "config.h"
#include "stdlib.h"
#include "usb_host.h"
#include "cmsis_os.h"
#include "marlin_client.h"
#include "screen_print_preview.h"
#include "print_utils.h"
#include "screens.h"

#include "../Marlin/src/gcode/queue.h"
#include "../Marlin/src/gcode/lcd/M73_PE.h"

#ifndef MAXPATHNAMELENGTH
    #define MAXPATHNAMELENGTH F_MAXPATHNAMELENGTH
#endif

#define LOG_ERROR(...) _dbg3("FILEBROWSER ERROR: " __VA_ARGS__)

typedef struct
{
    window_frame_t root;

    window_header_t header;
    window_file_list_t w_filelist;
} screen_filebrowser_data_t;

#define pd ((screen_filebrowser_data_t *)screen->pdata)

// Default value could be rewrite from eeprom settings
static WF_Sort_t screen_filebrowser_sort = WF_SORT_BY_TIME;

/// To save first/top visible item in the file browser
/// This is something else than the selected file for print
/// This is used to restore the content of the browser into previous state including the layout
constexpr unsigned int SFN_len = 13;
static char firstVisibleSFN[SFN_len] = "";

static void screen_filebrowser_init(screen_t *screen) {
    // TODO: load screen_filebrowser_sort from eeprom
    // FIXME: this could crash with very fast insert and eject, status_header will fix this
    marlin_event_clr(MARLIN_EVT_MediaRemoved); // when screen is open, USB must be inserted

    int16_t id;
    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pd->root));
    window_disable(root); // hack for do not change capture

    id = window_create_ptr(WINDOW_CLS_HEADER, root, gui_defaults.header_sz, &(pd->header));
    p_window_header_set_icon(&(pd->header), IDR_PNG_filescreen_icon_folder);
    p_window_header_set_text(&(pd->header), "SELECT FILE");

    window_file_list_t *filelist = &(pd->w_filelist);

    id = window_create_ptr(WINDOW_CLS_FILE_LIST, root,
        rect_ui16(10, 32, 220, 278),
        filelist);

    // initialize the directory (and selected file) from marlin_vars
    marlin_vars_t *vars = marlin_vars();
    // here the strncpy is meant to be - need the rest of the buffer zeroed
    strncpy(filelist->sfn_path, vars->media_SFN_path, sizeof(filelist->sfn_path));
    // ensure null character at the end no matter what
    filelist->sfn_path[sizeof(filelist->sfn_path) - 1] = '\0';
    // cut by the filename to retain only the directory path
    char *c = strrchr(filelist->sfn_path, '/');
    *c = 0; // even if we didn't find the '/', c will point to valid memory
    // Moreover - the next characters after c contain the filename, which I want to start my cursor at!
    window_file_list_load(filelist, screen_filebrowser_sort, c + 1, firstVisibleSFN);
    // window_file_set_item_index(filelist, 1); // this is automagically done in the window file list
    window_set_capture(id); // hack for do not change capture
    window_set_focus(id);   // hack for do not change capture
}

static void screen_filebrowser_done(_screen_t *screen) {
    window_destroy(pd->root.win.id);
}

static void screen_filebrowser_draw(screen_t *screen) {}

static void on_print_preview_action(print_preview_action_t action) {
    if (action == PRINT_PREVIEW_ACTION_BACK) {
        screen_close(); // close the print preview
    } else if (action == PRINT_PREVIEW_ACTION_PRINT) {
        screen_close(); // close the print preview
        screen_close(); // close the file browser
        print_begin(screen_print_preview_get_gcode_filepath());
        screen_open(get_scr_printing()->id);
    }
}

static void screen_filebrowser_clear_firstVisibleSFN(marlin_vars_t *vars) {
    vars->media_SFN_path[0] = '/';
    vars->media_SFN_path[1] = 0;
    firstVisibleSFN[0] = 0; // clear the last top item
}

static int screen_filebrowser_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    marlin_vars_t *vars = marlin_vars();
    if (marlin_event_clr(MARLIN_EVT_MediaRemoved)) { // close screen when media removed
        screen_filebrowser_clear_firstVisibleSFN(vars);
        screen_close();
        return 1;
    }

    window_header_events(&(pd->header));

    window_file_list_t *filelist = &(pd->w_filelist);

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    static const char dirUp[] = "..";
    static const char slash = '/';

    bool currentIsFile;
    const char *currentSFN = window_file_current_SFN(filelist, &currentIsFile);

    if (!strcmp(currentSFN, dirUp) && window_file_list_path_is_root(filelist->sfn_path)) {
        screen_filebrowser_clear_firstVisibleSFN(vars);
        screen_close();
        return 1;
    }

    size_t sfnPathLen = strlen(filelist->sfn_path);
    if ((sfnPathLen + strlen(currentSFN) + 1) >= MAXPATHNAMELENGTH) {
        LOG_ERROR("path too long");
        return 0;
    }
    if (!currentIsFile) {                // directory selected
        if (strcmp(currentSFN, dirUp)) { // not same -> not ..
            // append the dir name at the end of sfnPath
            if (filelist->sfn_path[sfnPathLen - 1] != slash) {
                filelist->sfn_path[sfnPathLen++] = slash;
            }
            strlcpy(filelist->sfn_path + sfnPathLen, currentSFN, FILE_PATH_MAX_LEN - sfnPathLen);
        } else {
            char *last = strrchr(filelist->sfn_path, slash);
            if (last == filelist->sfn_path) {
                // reached top level dir - ensure it only contains a slash
                filelist->sfn_path[0] = slash;
                filelist->sfn_path[1] = 0;
            } else {
                *last = '\0'; // truncate the string after the last "/"
            }
        }
        window_file_list_load(filelist, screen_filebrowser_sort, nullptr, nullptr);

        // @@TODO we want to print the LFN of the dir name, which is very hard to do right now
        // However, the text is not visible on the screen yet...
        window_set_text(pd->header.win.id, strrchr(filelist->sfn_path, '/'));

    } else { // print the file
        if (vars->media_LFN && vars->media_SFN_path) {
            int written;
            if (window_file_list_path_is_root(filelist->sfn_path)) {
                written = snprintf(vars->media_SFN_path, FILE_PATH_MAX_LEN, "/%s", currentSFN);
            } else {
                written = snprintf(vars->media_SFN_path, FILE_PATH_MAX_LEN, "%s/%s", filelist->sfn_path, currentSFN);
            }
            if (written < 0 || written >= (int)FILE_PATH_MAX_LEN) {
                LOG_ERROR("failed to prepare file path for print");
                return 0;
            }

            // displayed text - can be a 8.3 DOS name or a LFN
            strlcpy(vars->media_LFN, window_file_current_LFN(filelist, &currentIsFile), FILE_NAME_MAX_LEN);
            // save the top browser item
            strlcpy(firstVisibleSFN, window_file_list_top_item_SFN(filelist), SFN_len);

            screen_print_preview_set_on_action(on_print_preview_action);
            screen_print_preview_set_gcode_filepath(vars->media_SFN_path);
            screen_print_preview_set_gcode_filename(vars->media_LFN);
            screen_open(get_scr_print_preview()->id);

            return 1;
        }
    }

    return 0;
}

static screen_t screen_filebrowser = {
    0,
    0,
    screen_filebrowser_init,
    screen_filebrowser_done,
    screen_filebrowser_draw,
    screen_filebrowser_event,
    sizeof(screen_filebrowser_data_t),
    nullptr
};

screen_t *const get_scr_filebrowser() { return &screen_filebrowser; }
