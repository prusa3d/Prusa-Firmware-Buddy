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
    uint8_t alignment;
    int count;                              // total number of files/entries in a dir
    int index;                              // selected index - cursor position within the visible items
    char altpath[F_MAXPATHNAMELENGTH - 12]; // this is a path where we start the file dialog
    void *ldv;                              // I'm a C-pig and I need a pointer to my LazyDirView class instance ... subject to change when this gets rewritten to C++
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

extern void window_file_list_load(window_file_list_t *window, WF_Sort_t sort);

extern void window_file_set_item_index(window_file_list_t *window, int index);

extern const char *window_file_current_fname(window_file_list_t *window, bool *isFile);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* API_WINDOW_FILE_LIST_H_ */
