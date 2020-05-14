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

static const char *filters[] = { "*.gcode", "*.gco", "*.g" };
static const size_t filt_cnt = sizeof(filters) / sizeof(const char *);

static void screen_filebrowser_init(screen_t *screen) {
    // TODO: load screen_filebrowser_sort from eeprom
    // FIXME: this could crash with very fast insert and eject, status_header will fix this
    marlin_event_clr(MARLIN_EVT_MediaRemoved); // when screen is open, USB must be inserted

    int16_t id;
    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pd->root));
    window_disable(root); // hack for do not change capture

    id = window_create_ptr(WINDOW_CLS_HEADER, root,
        rect_ui16(0, 0, 240, 31), &(pd->header));
    p_window_header_set_icon(&(pd->header), IDR_PNG_filescreen_icon_folder);
    p_window_header_set_text(&(pd->header), "SELECT FILE");

    id = window_create_ptr(WINDOW_CLS_FILE_LIST, root,
        rect_ui16(10, 32, 220, 278),
        &(pd->w_filelist));
    window_file_list_load(&(pd->w_filelist), screen_filebrowser_sort);
    window_file_set_item_index(&(pd->w_filelist), 1);
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

static int screen_filebrowser_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {
    if (marlin_event_clr(MARLIN_EVT_MediaRemoved)) { // close screen when media removed
        screen_close();
        return 1;
    }

    window_header_events(&(pd->header));

    window_file_list_t *filelist = &(pd->w_filelist);

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    bool currentIsFile;
    // displayed text - can be a 8.3 DOS name or a LFN
    const char *currentFName = window_file_current_fname(filelist, &currentIsFile);

    //there must be fname, not altname
    if (!strcmp(currentFName, "..") && !strcmp(filelist->altpath, "/")) {
        screen_close();
        return 1;
    }

    size_t altPathLen = strlen(filelist->altpath);
    if ((altPathLen + strlen(currentFName) + 1) >= MAXPATHNAMELENGTH) {
        LOG_ERROR("path too long");
        return 0;
    }
    if (!currentIsFile) {                 // directory selected
        if (strcmp(currentFName, "..")) { // not same -> not ..
            // append the dir name at the end of altPath
            if (filelist->altpath[altPathLen - 1] != '/') {
                filelist->altpath[altPathLen++] = '/';
            }
            strcpy(filelist->altpath + altPathLen, currentFName);
        } else {
            char *last = strrchr(filelist->altpath, '/');
            if (last == filelist->altpath) {
                strcpy(last, "/"); // reached top level dir
            } else {
                *last = '\0'; // truncate the string after the last "/"
            }
        }
        window_file_list_load(filelist, screen_filebrowser_sort);
        window_set_text(pd->header.win.id, strrchr(filelist->altpath, '/'));
    } else { // print the file

        marlin_vars_t *vars = marlin_vars();

        if (vars->media_file_name && vars->media_file_path) {

            int written;
            if (!strcmp(filelist->altpath, "/"))
                written = snprintf(vars->media_file_path, FILE_PATH_MAX_LEN,
                    "/%s", currentFName);
            else
                written = snprintf(vars->media_file_path, FILE_PATH_MAX_LEN,
                    "%s/%s", filelist->altpath, currentFName);

            if (written < 0 || written >= (int)FILE_PATH_MAX_LEN) {
                LOG_ERROR("failed to prepare file path for print");
                return 0;
            }

            strcpy(vars->media_file_name, currentFName);

            screen_print_preview_set_on_action(on_print_preview_action);
            screen_print_preview_set_gcode_filepath(vars->media_file_path);
            screen_print_preview_set_gcode_filename(vars->media_file_name);
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

extern "C" screen_t *const get_scr_filebrowser() { return &screen_filebrowser; }
