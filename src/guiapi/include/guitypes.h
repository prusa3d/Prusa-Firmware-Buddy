// guitypes.h
#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

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

typedef uint32_t color_t;

typedef struct _point_ui16_t {
    uint16_t x;
    uint16_t y;
} point_ui16_t;

typedef struct _size_ui16_t {
    uint16_t w;
    uint16_t h;
} size_ui16_t;

typedef struct _rect_ui16_t {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} rect_ui16_t;

typedef struct _font_t {
    uint8_t w;    //char width [pixels]
    uint8_t h;    //char height [pixels]
    uint8_t bpr;  //bytes per row
    uint32_t flg; //flags
    void *pcs;    //charset data pointer
    char asc_min; //min ascii code (first character)
    char asc_max; //max ascii code (last character)
} font_t;

typedef struct _padding_ui8_t {
    uint8_t left;
    uint8_t top;
    uint8_t right;
    uint8_t bottom;
} padding_ui8_t;

typedef struct _bitmap_t {
    uint16_t w;  //bitmap width [pixels]
    uint16_t h;  //bitmap height [pixels]
    uint8_t bpp; //bits per pixel
    uint8_t bpr; //bytes per row
    void *ppx;   //pixel data pointer
} bitmap_t;

typedef struct _resource_entry_t {
    const uint8_t *ptr;  // 4 bytes - pointer
    const uint16_t size; // 2 bytes - data size
} resource_entry_t;

typedef struct _gui_defaults_t {
    color_t color_back;
    color_t color_text;
    color_t color_disabled;
    font_t *font;
    font_t *font_big;
    padding_ui8_t padding;
    uint8_t alignment;
    rect_ui16_t header_sz;           // default header location & size
    rect_ui16_t scr_body_sz;         // default screen body location & size
    rect_ui16_t scr_body_no_foot_sz; // screen body without footer location & size
    rect_ui16_t scr_fullscreen;      // full screen body without footer & header location & size
    rect_ui16_t footer_sz;           // default footer location & size
    uint8_t btn_h;                   // default button height
    uint8_t btn_spacing;             // default button spacing
    uint8_t frame_width;             // default frame width
} gui_defaults_t;

static inline uint16_t swap_ui16(uint16_t val) {
    return (val >> 8) | ((val & 0xff) << 8);
}

static inline uint16_t swap_ui32(uint32_t val) {
    return (val >> 16) | ((val & 0xffff) << 16);
}

static inline color_t color_rgb(const uint8_t r, const uint8_t g, const uint8_t b) {
    return r | ((uint32_t)g << 8) | ((uint32_t)b << 16);
}

static inline uint16_t color_to_565(color_t clr) {
    return swap_ui16(((clr >> 19) & 0x001f) | ((clr >> 5) & 0x07e0) | ((clr << 8) & 0xf800));
}

static inline color_t color_from_565(uint16_t clr565) {
    //TODO
    return 0;
}

static inline color_t color_alpha(const color_t clr0, const color_t clr1, const uint8_t alpha) {
    const uint8_t r0 = clr0 & 0xff;
    const uint8_t g0 = (clr0 >> 8) & 0xff;
    const uint8_t b0 = (clr0 >> 16) & 0xff;
    const uint8_t r1 = clr1 & 0xff;
    const uint8_t g1 = (clr1 >> 8) & 0xff;
    const uint8_t b1 = (clr1 >> 16) & 0xff;
    const uint8_t r = ((255 - alpha) * r0 + alpha * r1) / 255;
    const uint8_t g = ((255 - alpha) * g0 + alpha * g1) / 255;
    const uint8_t b = ((255 - alpha) * b0 + alpha * b1) / 255;
    return color_rgb(r, g, b);
}

static inline point_ui16_t point_ui16(uint16_t x, uint16_t y) {
    point_ui16_t point = { x, y };
    return point;
}

static inline size_ui16_t size_ui16(uint16_t w, uint16_t h) {
    size_ui16_t size = { w, h };
    return size;
}

static inline rect_ui16_t rect_ui16(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    rect_ui16_t rect = { x, y, w, h };
    return rect;
}

static inline padding_ui8_t padding_ui8(uint8_t l, uint8_t t, uint8_t r, uint8_t b) {
    padding_ui8_t padding = { l, t, r, b };
    return padding;
}

static inline int point_in_rect_ui16(point_ui16_t pt, rect_ui16_t rc) {
    return ((pt.x >= rc.x) && (pt.x < (rc.x + rc.w)) && (pt.y >= rc.y) && (pt.y < (rc.y + rc.h))) ? 1 : 0;
}

static inline int rect_in_rect_ui16(rect_ui16_t rc, rect_ui16_t rc1) {
    return ((rc.x >= rc1.x) && ((rc.x + rc.w) <= (rc1.x + rc1.w)) && (rc.y >= rc1.y) && ((rc.y + rc.h) <= (rc1.y + rc1.h))) ? 1 : 0;
}

static inline int rect_empty_ui16(rect_ui16_t rc) {
    return ((rc.w == 0) || (rc.h == 0)) ? 1 : 0;
}

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern rect_ui16_t rect_intersect_ui16(rect_ui16_t rc1, rect_ui16_t rc2);

extern rect_ui16_t rect_ui16_add_padding_ui8(rect_ui16_t rc, padding_ui8_t pad);

extern rect_ui16_t rect_ui16_sub_padding_ui8(rect_ui16_t rc, padding_ui8_t pad);

extern rect_ui16_t rect_align_ui16(rect_ui16_t rc, rect_ui16_t rc1, uint8_t align);

extern point_ui16_t icon_meas(const uint8_t *pi);
extern size_ui16_t icon_size(const uint8_t *pi);

extern const uint8_t *resource_ptr(uint16_t id);

extern uint16_t resource_size(uint16_t id);

extern FILE *resource_fopen(uint16_t id, const char *opentype);

extern font_t *resource_font(uint16_t id);

#ifdef __cplusplus
}
#endif //__cplusplus

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
