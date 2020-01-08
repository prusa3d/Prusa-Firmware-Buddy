/*
 * window_file_list.h
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#ifndef API_WINDOW_FILE_LIST_H_
#define API_WINDOW_FILE_LIST_H_

#include "window.h"
#include "display_helper.h"
#include "ffconf.h"
#include "ff.h"

/*
 * Using limits from marlin:
 * SDSORT_LIMIT					64
 * FOLDER_SORTING				-1 dir above
 * FILENAME_LENGTH				13
 * LONG_FILENAME_LENGTH			13 * 5 + 1
 * MAX_DIR_DEPTH				10
 * MAXDIRNAMELENGTH   			8
 * MAXPATHNAMELENGTH			(1 + (MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FILENAME_LENGTH)
 * */

#ifndef SDSORT_LIMIT
    #define SDSORT_LIMIT 64 // DOS folder name size
#endif

#ifndef FOLDER_SORTING
    #define FOLDER_SORTING -1 // DOS folder name size
#endif

#ifndef MAX_DIR_DEPTH
    #define MAX_DIR_DEPTH 10 // Maximum folder depth
#endif

#ifndef MAXDIRNAMELENGTH
    #define F_MAXDIRNAMELENGTH _MAX_LFN + 1
#endif

#ifndef MAXPATHNAMELENGTH
    #define F_MAXPATHNAMELENGTH (1 + (F_MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + _MAX_LFN)
#endif

typedef struct _window_file_list_t window_file_list_t;

#pragma pack(push)
#pragma pack(1)

typedef struct _window_class_file_list_t {
    window_class_t cls;
} window_class_file_list_t;
/*
typedef struct _file_item_t {
	char name[_MAX_LFN];
	uint8_t dir;
	WORD ftime;
	WORD fdate;
} file_item_t;
*/
typedef struct _window_file_list_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    padding_ui8_t padding;
    uint8_t alignment;
    int count;
    int index;
    int top_index;
    //char path[F_MAXPATHNAMELENGTH-_MAX_LFN];
    char altpath[F_MAXPATHNAMELENGTH - 12];
    FILINFO file_items[SDSORT_LIMIT];
    txtroll_t roll;
    uint8_t last_index;
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

extern void window_file_list_load(window_file_list_t *window, const char **filters,
    size_t filters_cnt, WF_Sort_t sort);

extern void window_file_set_item_index(window_file_list_t *window, int index);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* API_WINDOW_FILE_LIST_H_ */
