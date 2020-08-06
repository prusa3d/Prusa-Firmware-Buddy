// guitypes.h
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//alignment constants
#define ALIGN_LEFT          0x00
#define ALIGN_HCENTER       0x01
#define ALIGN_RIGHT         0x02
#define ALIGN_HMASK         0x03
#define ALIGN_TOP           0x00
#define ALIGN_VCENTER       0x10
#define ALIGN_BOTTOM        0x20
#define ALIGN_VMASK         0x30
#define ALIGN_MASK          0x33
#define ALIGN_CENTER        (ALIGN_HCENTER | ALIGN_VCENTER)
#define ALIGN_LEFT_TOP      (ALIGN_LEFT | ALIGN_TOP)
#define ALIGN_LEFT_CENTER   (ALIGN_LEFT | ALIGN_VCENTER)
#define ALIGN_LEFT_BOTTOM   (ALIGN_LEFT | ALIGN_BOTTOM)
#define ALIGN_RIGHT_TOP     (ALIGN_RIGHT | ALIGN_TOP)
#define ALIGN_RIGHT_CENTER  (ALIGN_RIGHT | ALIGN_VCENTER)
#define ALIGN_RIGHT_BOTTOM  (ALIGN_RIGHT | ALIGN_BOTTOM)
#define ALIGN_CENTER_TOP    (ALIGN_HCENTER | ALIGN_TOP)
#define ALIGN_CENTER_BOTTOM (ALIGN_HCENTER | ALIGN_BOTTOM)

//raster operation function constants
#define ROPFN_COPY    0x00 //copy (no operation)
#define ROPFN_INVERT  0x01 //invert
#define ROPFN_SWAPBW  0x02 //swap black-white
#define ROPFN_DISABLE 0x04 //disables (darker colors)

//font flags
#define FONT_FLG_SWAP 0x00000001 // swap low/high byte
#define FONT_FLG_LSBF 0x00000002 // LSB first

//color constants
#define COLOR_BLACK     0x00000000L
#define COLOR_WHITE     0x00ffffffL
#define COLOR_RED       0x000000ffL
#define COLOR_RED_ALERT 0x002646e7L
#define COLOR_LIME      0x0000ff00L
#define COLOR_BLUE      0x00ff0000L
#define COLOR_YELLOW    0x0000ffffL
#define COLOR_CYAN      0x00ffff00L
#define COLOR_MAGENTA   0x00ff00ffL
#define COLOR_SILVER    0x00c0c0c0L
#define COLOR_GRAY      0x00808080L
#define COLOR_MAROON    0x00000080L
#define COLOR_OLIVE     0x00008080L
#define COLOR_GREEN     0x00008000L
#define COLOR_PURPLE    0x00800080L
#define COLOR_TEAL      0x00808000L
#define COLOR_NAVY      0x00800000L
#define COLOR_ORANGE    0x001B65F8L

#define COLOR_DARK_KHAKI 0x006BD7DBL

typedef struct _resource_entry_t {
    const uint8_t *ptr;  // 4 bytes - pointer
    const uint16_t size; // 2 bytes - data size
} resource_entry_t;

typedef struct _font_t {
    uint8_t w;    //char width [pixels]
    uint8_t h;    //char height [pixels]
    uint8_t bpr;  //bytes per row
    uint32_t flg; //flags
    void *pcs;    //charset data pointer
    char asc_min; //min ascii code (first character)
    char asc_max; //max ascii code (last character)
} font_t;

//resource type definition
#define RESOURCE_TYPE_RAW 0 //raw binary resource
#define RESOURCE_TYPE_TXT 1 //text resource
#define RESOURCE_TYPE_FNT 2 //font resource
#define RESOURCE_TYPE_BMP 3 //bitmap picture resource
#define RESOURCE_TYPE_PNG 4 //png picture resource

//resource table macros
#define RESOURCE_TABLE_BEGIN const resource_entry_t resource_table[] = {
#define RESOURCE_TABLE_END                                       \
    }                                                            \
    ;                                                            \
    const uint16_t resource_table_size = sizeof(resource_table); \
    const uint16_t resource_count = sizeof(resource_table) / sizeof(resource_entry_t);

#define RESOURCE_ENTRY_NUL()  { 0, 0 },
#define RESOURCE_ENTRY_PNG(v) { v, sizeof(v) },
#define RESOURCE_ENTRY_FNT(v) { (uint8_t *)&v, sizeof(font_t) },
