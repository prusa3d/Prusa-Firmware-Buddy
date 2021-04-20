#include "lwip/apps/fs.h"
#include "lwip/def.h"

#define file_NULL (struct fsdata_file *)NULL

#ifndef FS_FILE_FLAGS_HEADER_INCLUDED
    #define FS_FILE_FLAGS_HEADER_INCLUDED 1
#endif
#ifndef FS_FILE_FLAGS_HEADER_PERSISTENT
    #define FS_FILE_FLAGS_HEADER_PERSISTENT 0
#endif
/* FSDATA_FILE_ALIGNMENT: 0=off, 1=by variable, 2=by include */
#ifndef FSDATA_FILE_ALIGNMENT
    #define FSDATA_FILE_ALIGNMENT 0
#endif
#ifndef FSDATA_ALIGN_PRE
    #define FSDATA_ALIGN_PRE
#endif
#ifndef FSDATA_ALIGN_POST
    #define FSDATA_ALIGN_POST
#endif
#if FSDATA_FILE_ALIGNMENT == 2
    #include "fsdata_alignment.h"
#endif
#if FSDATA_FILE_ALIGNMENT == 1
static const unsigned int dummy_align__favicon_ico = 0;
#endif
static const unsigned char FSDATA_ALIGN_PRE data__favicon_ico[] FSDATA_ALIGN_POST = {
};

#if FSDATA_FILE_ALIGNMENT == 1
static const unsigned int dummy_align__index_html = 1;
#endif
static const unsigned char FSDATA_ALIGN_PRE data__index_html[] FSDATA_ALIGN_POST = {
};

#if FSDATA_FILE_ALIGNMENT == 1
static const unsigned int dummy_align__main_b322e2869b73e803bb67_css = 2;
#endif
static const unsigned char FSDATA_ALIGN_PRE data__main_b322e2869b73e803bb67_css[] FSDATA_ALIGN_POST = {
};

#if FSDATA_FILE_ALIGNMENT == 1
static const unsigned int dummy_align__main_b322e2869b73e803bb67_js = 3;
#endif
static const unsigned char FSDATA_ALIGN_PRE data__main_b322e2869b73e803bb67_js[] FSDATA_ALIGN_POST = {
};

const struct fsdata_file file__favicon_ico[] = { {
    file_NULL,
    data__favicon_ico,
    data__favicon_ico + 16,
    sizeof(data__favicon_ico) - 16,
    0,
} };

const struct fsdata_file file__index_html[] = { {
    file__favicon_ico,
    data__index_html,
    data__index_html + 12,
    sizeof(data__index_html) - 12,
    0,
} };

const struct fsdata_file file__main_b322e2869b73e803bb67_css[] = { {
    file__index_html,
    data__main_b322e2869b73e803bb67_css,
    data__main_b322e2869b73e803bb67_css + 32,
    sizeof(data__main_b322e2869b73e803bb67_css) - 32,
    0,
} };

const struct fsdata_file file__main_b322e2869b73e803bb67_js[] = { {
    file__main_b322e2869b73e803bb67_css,
    data__main_b322e2869b73e803bb67_js,
    data__main_b322e2869b73e803bb67_js + 32,
    sizeof(data__main_b322e2869b73e803bb67_js) - 32,
    0,
} };

#define FS_ROOT     file__main_b322e2869b73e803bb67_js
#define FS_NUMFILES 4
