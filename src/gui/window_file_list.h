/*
 * window_file_list.h
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#ifndef API_WINDOW_FILE_LIST_H_
#define API_WINDOW_FILE_LIST_H_

#include "window.h"
#include "ff.h"
#include <stdbool.h>
#include "file_list_defs.h"
#include "display_helper.h"
#include "../common/marlin_vars.h" // for FILE_PATH_MAX_LEN

typedef struct _window_file_list_t window_file_list_t;

#pragma pack(push)
#pragma pack(1)

typedef struct _window_class_file_list_t {
    window_class_t cls;
} window_class_file_list_t;

typedef struct _window_file_list_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    padding_ui8_t padding;
    txtroll_t roll;
    uint8_t alignment;
    uint8_t last_index;
    int count;                        // total number of files/entries in a dir
    int index;                        // selected index - cursor position within the visible items
    char sfn_path[FILE_PATH_MAX_LEN]; // this is a Short-File-Name path where we start the file dialog
    void *ldv;                        // I'm a C-pig and I need a pointer to my LazyDirView class instance ... subject to change when this gets rewritten to C++
} window_file_list_t;

#pragma pack(pop)

// This enum value is stored to eeprom as file sort settings
typedef enum {
    WF_SORT_BY_TIME,
    WF_SORT_BY_NAME

} WF_Sort_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int16_t WINDOW_CLS_FILE_LIST;

extern const window_class_file_list_t window_class_file_list;

extern void window_file_list_load(window_file_list_t *window, WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN);

extern void window_file_set_item_index(window_file_list_t *window, int index);

extern const char *window_file_list_top_item_SFN(window_file_list_t *window);
extern const char *window_file_current_LFN(window_file_list_t *window, bool *isFile);
extern const char *window_file_current_SFN(window_file_list_t *window, bool *isFile);

/// @return true if path is either empty or contains just a "/"
extern bool window_file_list_path_is_root(const char *path);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* API_WINDOW_FILE_LIST_H_ */
