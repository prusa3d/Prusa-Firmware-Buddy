// guitypes.h
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//alignment constants
enum {
    ALIGN_LEFT = 0x00,
    ALIGN_HCENTER = 0x01,
    ALIGN_RIGHT = 0x02,
    ALIGN_HMASK = 0x03,
    ALIGN_TOP = 0x00,
    ALIGN_VCENTER = 0x10,
    ALIGN_BOTTOM = 0x20,
    ALIGN_VMASK = 0x30,
    ALIGN_MASK = 0x33,

    ALIGN_CENTER = ALIGN_HCENTER | ALIGN_VCENTER,
    ALIGN_LEFT_TOP = ALIGN_LEFT | ALIGN_TOP,
    ALIGN_LEFT_CENTER = ALIGN_LEFT | ALIGN_VCENTER,
    ALIGN_LEFT_BOTTOM = ALIGN_LEFT | ALIGN_BOTTOM,
    ALIGN_RIGHT_TOP = ALIGN_RIGHT | ALIGN_TOP,
    ALIGN_RIGHT_CENTER = ALIGN_RIGHT | ALIGN_VCENTER,
    ALIGN_RIGHT_BOTTOM = ALIGN_RIGHT | ALIGN_BOTTOM,
    ALIGN_CENTER_TOP = ALIGN_HCENTER | ALIGN_TOP,
    ALIGN_CENTER_BOTTOM = ALIGN_HCENTER | ALIGN_BOTTOM,
};

//raster operation function constants
enum {
    ROPFN_COPY = 0x00,    //copy (no operation)
    ROPFN_INVERT = 0x01,  //invert
    ROPFN_SWAPBW = 0x02,  //swap black-white
    ROPFN_DISABLE = 0x04, //disables (darker colors)
};

//font flags
enum {
    FONT_FLG_SWAP = 0x00000001, // swap low/high byte
    FONT_FLG_LSBF = 0x02,       // LSB first
};

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
