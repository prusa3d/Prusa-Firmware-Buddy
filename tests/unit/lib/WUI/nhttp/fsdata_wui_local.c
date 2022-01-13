#include <lwip/apps/fs.h>
#include <lwip/def.h>
#define file_NULL (struct fsdata_file *)NULL

#ifndef FS_FILE_FLAGS_HEADER_INCLUDED
    #define FS_FILE_FLAGS_HEADER_INCLUDED 1
#endif
#ifndef FS_FILE_FLAGS_HEADER_PERSISTENT
    #define FS_FILE_FLAGS_HEADER_PERSISTENT 0
#endif
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

static const unsigned char FSDATA_ALIGN_PRE data__index_html[] FSDATA_ALIGN_POST = {
    0x2f,
    0x69,
    0x6e,
    0x64,
    0x65,
    0x78,
    0x2e,
    0x68,
    0x74,
    0x6d,
    0x6c,
    0x0,
    0x1f,
    0x8b,
    0x8,
    0x0,
    0x95,
    0xc4,
    0xb0,
    0x61,
    0x2,
    0xff,
    0xb3,
    0xc9,
    0x28,
    0xc9,
    0xcd,
    0xb1,
    0xe3,
    0x52,
    0x0,
    0x2,
    0x9b,
    0xa4,
    0xfc,
    0x94,
    0x4a,
    0x3b,
    0x8f,
    0xd4,
    0x9c,
    0x9c,
    0x7c,
    0x85,
    0xf2,
    0xfc,
    0xa2,
    0x9c,
    0x14,
    0x45,
    0x2e,
    0x1b,
    0x7d,
    0x88,
    0x34,
    0x0,
    0x7,
    0x7d,
    0x1,
    0x41,
    0x26,
    0x0,
    0x0,
    0x0,
};

const struct fsdata_file file__index_html[] = { {
    file_NULL,
    data__index_html,
    data__index_html + 12,
    sizeof(data__index_html) - 12,
    0,
} };

#define FS_NUMFILES 1
#define FS_ROOT     file__index_html
