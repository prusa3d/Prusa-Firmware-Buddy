// guitypes.h
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef uint32_t color_t;

//color constants
static const color_t COLOR_BLACK = 0x00000000L;
static const color_t COLOR_WHITE = 0x00ffffffL;
static const color_t COLOR_RED = 0x000000ffL;
static const color_t COLOR_RED_ALERT = 0x002646e7L;
static const color_t COLOR_LIME = 0x0000ff00L;
static const color_t COLOR_BLUE = 0x00ff0000L;
static const color_t COLOR_AZURE = 0x00ff9d12L;
static const color_t COLOR_YELLOW = 0x0000ffffL;
static const color_t COLOR_CYAN = 0x00ffff00L;
static const color_t COLOR_MAGENTA = 0x00ff00ffL;
static const color_t COLOR_SILVER = 0x00c0c0c0L;
static const color_t COLOR_GRAY = 0x00808080L;
static const color_t COLOR_DARK_GRAY = 0x005B5B5BL;
static const color_t COLOR_MAROON = 0x00000080L;
static const color_t COLOR_OLIVE = 0x00008080L;
static const color_t COLOR_GREEN = 0x00008000L;
static const color_t COLOR_DARK_GREEN = 0x00006000L;
static const color_t COLOR_LIGHT_GREEN = 0x0040b040L;
static const color_t COLOR_PURPLE = 0x00800080L;
static const color_t COLOR_TEAL = 0x00808000L;
static const color_t COLOR_NAVY = 0x00800000L;
static const color_t COLOR_ORANGE = 0x001B65F8L;
static const color_t COLOR_DARK_KHAKI = 0x006BD7DBL;

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
enum {
    RESOURCE_TYPE_RAW, //raw binary resource
    RESOURCE_TYPE_TXT, //text resource
    RESOURCE_TYPE_FNT, //font resource
    RESOURCE_TYPE_BMP, //bitmap picture resource
    RESOURCE_TYPE_PNG, //png picture resource
};

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
