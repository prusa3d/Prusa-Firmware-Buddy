/**
 * @file display_ex.cpp
 */
#include "display_ex.hpp"
#include <functional>
#include <cmath>
#include "guiconfig.h" //USE_ST7789 USE_ILI9488
#include <img_resources.hpp>
#include "display_math_helper.h"
#include "font_flags.hpp"
#include <bsod.h>

#ifdef USE_ST7789
    #include "st7789v.hpp"
/*****************************************************************************/
// st7789v specific variables objects and function aliases
static constexpr Rect16 DisplayClip() { return Rect16(0, 0, ST7789V_COLS, ST7789V_ROWS); }

inline uint16_t color_to_native(uint32_t clr) {
    return color_to_565(clr);
}

inline uint32_t color_from_native(uint16_t clr) {
    return color_from_565(clr);
}

// TDispBuffer configuration
static constexpr size_t FontMaxBitLen = 4; // used in mask and buffer size
using BuffDATA_TYPE = uint16_t; // type of buffer internally used in TDispBuffer
using BuffPTR_TYPE = uint16_t; // type of buffer internally used pointer (does not need to match BuffDATA_TYPE)
static constexpr size_t BuffNATIVE_PIXEL_SIZE = 2; // bytes per pixel (can be same or smaller than size of BuffDATA_TYPE)
static constexpr size_t STORE_FN_PIXEL_SIZE = 1; // TODO find out why it is != BuffNATIVE_PIXEL_SIZE

static constexpr size_t buffROWS = ST7789V_BUFF_ROWS;

uint8_t *display_ex_borrow_buffer() {
    return st7789v_borrow_buffer();
}

void display_ex_return_buffer() {
    st7789v_return_buffer();
}

uint32_t display_ex_buffer_pixel_size() {
    return st7789v_buffer_size() / BuffNATIVE_PIXEL_SIZE;
}

void display_ex_draw_from_buffer(point_ui16_t pt, uint16_t w, uint16_t h) {
    st7789v_draw_from_buffer(pt.x, pt.y, w, h);
}

void display_ex_clear(const color_t clr) {
    st7789v_clear(color_to_native(clr));
}

static inline void draw_qoi_ex_C(FILE *pf, uint16_t point_x, uint16_t point_y, uint32_t back_color, ropfn rop, Rect16 subrect) {
    st7789v_draw_qoi_ex(pf, point_x, point_y, back_color, rop.ConvertToC(), subrect);
}

static inline uint8_t *get_block_C(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    return st7789v_get_block(start_x, start_y, end_x, end_y);
}

static inline uint16_t get_pixel_directColor_C(uint16_t point_x, uint16_t point_y) {
    return st7789v_get_pixel_colorFormat565(point_x, point_y);
}

static inline void set_pixel_colorFormatNative(uint16_t point_x, uint16_t point_y, uint32_t nativeclr) {
    st7789v_set_pixel(point_x, point_y, nativeclr);
}

static inline void fill_rect_colorFormatNative(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t nativeclr) {
    st7789v_fill_rect_colorFormat565(rect_x, rect_y, rect_w, rect_h, nativeclr);
}
// end st7789v specific variables objects and function aliases
/*****************************************************************************/
#endif // USE_ST7789

#ifdef USE_ILI9488
    #include "ili9488.hpp"
/*****************************************************************************/
// ili9488 specific variables objects and function aliases
static constexpr Rect16 DisplayClip() { return Rect16(0, 0, ILI9488_COLS, ILI9488_ROWS); }

inline uint32_t color_to_native(uint32_t clr) {
    return color_to_666(clr);
}

inline uint32_t color_from_native(uint32_t clr) {
    return color_from_666(clr);
}

// TDispBuffer configuration
static constexpr size_t FontMaxBitLen = 4; // used in mask and buffer size
using BuffDATA_TYPE = uint32_t; // type of buffer internally used in TDispBuffer
using BuffPTR_TYPE = uint8_t; // type of buffer internally used pointer (does not need to match BuffDATA_TYPE)
static constexpr size_t BuffNATIVE_PIXEL_SIZE = 3; // bytes per pixel (can be same or smaller than size of BuffDATA_TYPE)
static constexpr size_t STORE_FN_PIXEL_SIZE = 3;
static constexpr size_t buffROWS = ILI9488_BUFF_ROWS;

uint8_t *display_ex_borrow_buffer() {
    return ili9488_borrow_buffer();
}

void display_ex_return_buffer() {
    ili9488_return_buffer();
}

uint32_t display_ex_buffer_pixel_size() {
    return ili9488_buffer_size() / BuffNATIVE_PIXEL_SIZE;
}

void display_ex_draw_from_buffer(point_ui16_t pt, uint16_t w, uint16_t h) {
    ili9488_draw_from_buffer(pt.x, pt.y, w, h);
}

void display_ex_clear(const color_t clr) {
    ili9488_clear(color_to_native(clr));
}

static inline void draw_qoi_ex_C(FILE *pf, uint16_t point_x, uint16_t point_y, uint32_t back_color, ropfn rop, Rect16 subrect) {
    ili9488_draw_qoi_ex(pf, point_x, point_y, back_color, rop.ConvertToC(), subrect);
}

static inline uint8_t *get_block_C(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    return ili9488_get_block(start_x, start_y, end_x, end_y);
}

static inline uint16_t get_pixel_directColor_C(uint16_t point_x, uint16_t point_y) {
    return ili9488_get_pixel_colorFormat666(point_x, point_y);
}

static inline void set_pixel_colorFormatNative(uint16_t point_x, uint16_t point_y, uint32_t nativeclr) {
    ili9488_set_pixel(point_x, point_y, nativeclr);
}

static inline void fill_rect_colorFormatNative(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t nativeclr) {
    ili9488_fill_rect_colorFormat666(rect_x, rect_y, rect_w, rect_h, nativeclr);
}

// end ili9488 specific variables objects and function aliases
/*****************************************************************************/
#endif // USE_ILI9488

#ifdef USE_MOCK_DISPLAY
    #include "mock_display.hpp"
/*****************************************************************************/
// mock_display specific variables objects and function aliases
static Rect16 DisplayClip() { return Rect16(0, 0, MockDisplay::Cols(), MockDisplay::Rows()); }

inline uint32_t color_to_native(uint32_t clr) {
    return clr;
}

inline uint32_t color_from_native(uint32_t clr) {
    return clr;
}

// TDispBuffer configuration
static constexpr size_t FontMaxBitLen = 8; // used in mask and buffer size
using BuffDATA_TYPE = uint32_t; // type of buffer internally used in TDispBuffer
using BuffPTR_TYPE = uint32_t; // type of buffer internally used pointer (does not need to match BuffDATA_TYPE)
static constexpr size_t BuffNATIVE_PIXEL_SIZE = 4; // bytes per pixel (can be same or smaller than size of BuffDATA_TYPE)
static constexpr size_t STORE_FN_PIXEL_SIZE = 1; // TODO find out why it is != BuffNATIVE_PIXEL_SIZE
static constexpr size_t buffROWS = 256; // TODO mock display has this value variable

uint8_t *display_ex_borrow_buffer() { return MockDisplay::Instance().getBuff(); }

void display_ex_return_buffer() {}

uint32_t display_ex_buffer_pixel_size() {
    return (uint32_t)MockDisplay::Cols() * (uint32_t)MockDisplay::BuffRows();
}

void display_ex_draw_from_buffer(point_ui16_t pt, uint16_t w, uint16_t h) {
    MockDisplay::Instance().drawFromBuff(pt, w, h);
}

void display_ex_clear(const color_t clr) {
    MockDisplay::Instance().clear(clr);
}

static inline void draw_qoi_ex_C(FILE *pf, uint16_t point_x, uint16_t point_y, uint32_t back_color, ropfn rop, Rect16 subrect) {
    // todo
}

static inline uint8_t *get_block_C(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    return MockDisplay::Instance().GetBlock(start_x, start_y, end_x, end_y);
}

static inline uint32_t get_pixel_directColor_C(uint16_t point_x, uint16_t point_y) {
    return MockDisplay::Instance().GetpixelNativeColor(point_x, point_y);
}

static inline void set_pixel_colorFormatNative(uint16_t point_x, uint16_t point_y, uint32_t nativeclr) {
    MockDisplay::Instance().SetpixelNativeColor(point_x, point_y, nativeclr);
}

static inline void fill_rect_colorFormatNative(uint16_t rect_x, uint16_t rect_y, uint16_t rect_w, uint16_t rect_h, uint32_t nativeclr) {
    MockDisplay::Instance().FillRectNativeColor(rect_x, rect_y, rect_w, rect_h, nativeclr);
}

// end mock_display specific variables objects and function aliases
/*****************************************************************************/
#endif // USE_MOCK_DISPLAY

static constexpr size_t BuffAlphaLen = (1 << FontMaxBitLen); // size of buffer for alpha channel 4bit font need 2^4 == 16 etc

// do not use directly, use DispBuffer instead
template <size_t LEN, class DATA_TYPE, class PTR_TYPE, uint32_t NATIVE_PIXEL_SIZE>
class TDispBuffer {
    PTR_TYPE *const p_start;
    PTR_TYPE *p;
    DATA_TYPE clr_native[LEN];
    const color_t clr_bg;
    const color_t clr_fg;

public:
    // pms (pixel mask) == number of combinations of font bits
    TDispBuffer(uint8_t pms, color_t clr_bg, color_t clr_fg)
        : p_start((PTR_TYPE *)display_ex_borrow_buffer())
        , p(p_start)
        , clr_bg(clr_bg)
        , clr_fg(clr_fg) {
        for (size_t i = 0; i < std::min(LEN, size_t(pms + 1)); i++) {
            clr_native[i] = color_to_native(color_alpha(clr_bg, clr_fg, 255 * i / pms));
        }
    }

    ~TDispBuffer() {
        display_ex_return_buffer();
    }

    void inline Insert(size_t pos) {
        *(DATA_TYPE *)p = clr_native[pos];
        p += NATIVE_PIXEL_SIZE / sizeof(BuffPTR_TYPE);
    }

    void inline OffsetInsert(size_t clr_pos, uint32_t offset) {
        PTR_TYPE *ptr = p_start + offset;
        if constexpr (sizeof(BuffDATA_TYPE) == sizeof(BuffPTR_TYPE)) {
            *(DATA_TYPE *)ptr = clr_native[clr_pos];
        } else {
            for (uint8_t i = 0; i < NATIVE_PIXEL_SIZE; i++) {
                *(ptr + i) = (PTR_TYPE)(clr_native[clr_pos] >> (i * 8));
            }
        }
    }
    void inline Draw(point_ui16_t pt, uint16_t w, uint16_t h) { display_ex_draw_from_buffer(pt, w, h); }
};
using DispBuffer = TDispBuffer<BuffAlphaLen, BuffDATA_TYPE, BuffPTR_TYPE, BuffNATIVE_PIXEL_SIZE>;

static inline void store_to_buffer(uint8_t *buffer, Rect16 rect, uint16_t artefact_width, color_t color) {
    uint8_t *buff = buffer + (rect.Top() * artefact_width + rect.Left()) * BuffNATIVE_PIXEL_SIZE;
    uint32_t clr = color_to_native(color);
    for (int i = 0; i < rect.Height(); i++) {
        int offset = i * artefact_width * BuffNATIVE_PIXEL_SIZE;
        for (int j = 0; j < rect.Width(); j++) {
            for (size_t NthByte = 0; NthByte < BuffNATIVE_PIXEL_SIZE; ++NthByte) {
                buff[offset + (BuffNATIVE_PIXEL_SIZE * j) + NthByte] = (uint8_t)(clr >> (8 * NthByte));
            }
        }
    }
    return;
}

static inline uint32_t get_pixel(uint16_t point_x, uint16_t point_y) {
    return color_from_native(get_pixel_directColor_C(point_x, point_y));
}

/// Draws a single character according to selected font
/// \param charX x-index of character in font bitmap
/// \param charY y-index of character in font bitmap
/// \param clr_bg background color
/// \param clr_fg font/foreground color
/// If font is not available for the character, solid rectangle will be drawn in background color
/// \returns true if character is available in the font and was drawn
bool display_ex_draw_char(point_ui16_t pt, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    display_ex_store_char_in_buffer(1, 0, charX, charY, pf, clr_bg, clr_fg);
    display_ex_draw_from_buffer(pt, pf->w, pf->h);
    return true;
}

void display_ex_store_char_in_buffer(uint16_t char_cnt, uint16_t curr_char_idx, uint8_t charX, uint8_t charY, const font_t *pf, color_t clr_bg, color_t clr_fg) {
    const uint16_t char_w = pf->w; // char width
    const uint16_t char_h = pf->h; // char height
    const uint8_t bpr = pf->bpr; // bytes per row
    const uint16_t bpc = bpr * char_h; // bytes per char
    const uint8_t bpp = 8 * bpr / char_w; // bits per pixel
    const uint8_t ppb = 8 / bpp; // pixels per byte
    const uint8_t pms = std::min(size_t((1 << bpp) - 1), BuffAlphaLen - 1); // pixel mask, cannot be bigger than array to store alpha channel combinations

    uint8_t *pch; // character data pointer
    uint8_t crd = 0; // current row byte data
    uint8_t rb; // row byte
    uint8_t *pc; // character data row pointer

    const font_flags flags(pf->flg);

    DispBuffer buff(pms, clr_bg, clr_fg);

    uint32_t chr = charY * 16 + charX; // compute character index in font
    uint32_t buffer_offset = 0; // buffer byte offset

    pch = (uint8_t *)(pf->pcs) + ((chr /*- pf->asc_min*/) * bpc);

    uint8_t pixel_size = STORE_FN_PIXEL_SIZE;

    for (uint16_t j = 0; j < char_h; j++) {
        pc = pch + j * bpr;
        buffer_offset = j * char_cnt * char_w * pixel_size + curr_char_idx * char_w * pixel_size;
        for (uint16_t i = 0; i < char_w; i++) {
            if ((i % ppb) == 0) {
                if (flags.swap == is_swap::yes) {
                    rb = (i / ppb) ^ 1;
                    crd = pch[rb + j * bpr];
                } else {
                    crd = *(pc++);
                }
            }
            if (flags.lsb == fnt_lsb::yes) {
                buff.OffsetInsert(crd & pms, buffer_offset + i * pixel_size);
                crd >>= bpp;
            } else {
                buff.OffsetInsert(crd >> (8 - bpp), buffer_offset + i * pixel_size);
                crd <<= bpp;
            }
        }
    }
}

/// Draws a rectangle boundary of defined color
void display_ex_draw_rect(Rect16 rc, color_t clr) {
    if (rc.IsEmpty()) {
        return;
    }

    point_i16_t pt0 = rc.TopLeft();
    point_i16_t pt1 = { int16_t(rc.Left() + rc.Width() - 1), rc.Top() };
    point_i16_t pt2 = { rc.Left(), int16_t(rc.Top() + rc.Height() - 1) };

    display_ex_fill_rect(Rect16(pt0, rc.Width(), 1), clr); // top
    display_ex_fill_rect(Rect16(pt0, 1, rc.Height()), clr); // left
    display_ex_fill_rect(Rect16(pt1, 1, rc.Height()), clr); // right
    display_ex_fill_rect(Rect16(pt2, rc.Width(), 1), clr); // bottom
}

void display_ex_fill_rect(Rect16 rc, color_t clr) {
    rc = rc.Intersection(DisplayClip());
    if (rc.IsEmpty()) {
        return;
    }
    const uint32_t native_color = color_to_native(clr);
    fill_rect_colorFormatNative(rc.Left(), rc.Top(), rc.Width(), rc.Height(), native_color);
}

/// Draws simple line (no antialiasing)
/// Both end points are drawn
void display_ex_draw_line(point_ui16_t pt0, point_ui16_t pt1, color_t clr) {
    const uint32_t native_color = color_to_native(clr);
    // todo check rectangle
    int n;
    const int dx = pt1.x - pt0.x; // X-axis dimension
    const int dy = pt1.y - pt0.y; // Y-axis dimension
    int cx = std::abs(dx);
    int cy = std::abs(dy);
    const int adx = cx; // absolute difference in x ( = width - 1)
    const int ady = cy; // absolute difference in y ( = height - 1)

    if ((adx == 0) || (ady == 0)) { // orthogonal line
        fill_rect_colorFormatNative(std::min(pt0.x, pt1.x), std::min(pt0.y, pt1.y), adx + 1, ady + 1, native_color);
        return;
    }

    const int sx = std::signbit(dx) ? -1 : 1; // X-axis direction, positive when left-to-right
    const int sy = std::signbit(dy) ? -1 : 1; // Y-axis direction, positive when bottom-to-top

    if (adx > ady) { // likely vertical line
        for (n = adx; n > 0; --n) {
            set_pixel_colorFormatNative(pt0.x, pt0.y, native_color);
            if ((cx -= cy) <= 0) {
                pt0.y += sy;
                cx += adx;
            }
            pt0.x += sx;
        }
        return;
    }

    if (adx < ady) { // likely horizontal line
        for (n = ady; n > 0; --n) {
            set_pixel_colorFormatNative(pt0.x, pt0.y, native_color);
            if ((cy -= cx) <= 0) {
                pt0.x += sx;
                cy += ady;
            }
            pt0.y += sy;
        }
        return;
    }

    // adx == ady => diagonal line
    for (n = adx; n > 0; --n) {
        set_pixel_colorFormatNative(pt0.x, pt0.y, native_color);
        pt0.x += sx;
        pt0.y += sy;
    }
}

color_t display_ex_get_pixel(point_ui16_t pt) {
    if (!DisplayClip().Contain(pt)) {
        return 0;
    }
    return get_pixel(pt.x, pt.y);
}

uint8_t *display_ex_get_block(point_ui16_t start, point_ui16_t end) {
    if (!DisplayClip().Contain(start) || !DisplayClip().Contain(end)) {
        return NULL;
    }
    return get_block_C(start.x, start.y, end.x, end.y);
}

// Draws rounded rect with only one pixel radius (draw_rounded_rect() process is not compatible with such a low radius)

void display_ex_draw_rounded_rect_rad1(Rect16 rect, color_t back, color_t front, uint8_t flag, uint8_t loop, color_t secondary_color) {
    uint8_t *buffer = display_ex_borrow_buffer();

    uint16_t h_left = rect.Height();
    for (uint8_t i = 0; i < loop; i++) { // If rectangle is higher than buffROWS (8 on ILI9488), it has to be separated
        uint8_t buff_rows_to_draw = (h_left < buffROWS ? h_left : buffROWS);
        // We paint whole rect with front color and then paint the round edge's complement in back color TO AVOID FLICKERING
        store_to_buffer(buffer, Rect16(0, 0, rect.Width(), buff_rows_to_draw), rect.Width(), front);

        if (i == 0) { // Draw background color over the top row (1 px on each side)
            if (flag & MIC_TOP_LEFT) {
                store_to_buffer(buffer, Rect16(0, 0, 1, 1), rect.Width(), flag & MIC_ALT_CL_TOP_LEFT ? secondary_color : back);
            }
            if (flag & MIC_TOP_RIGHT) {
                store_to_buffer(buffer, Rect16(rect.Width() - 1, 0, 1, 1), rect.Width(), flag & MIC_ALT_CL_TOP_RIGHT ? secondary_color : back);
            }
        }
        if (h_left == buff_rows_to_draw) { // Draw background color over the last row (1 px on each side)
            if (flag & MIC_BOT_LEFT) {
                store_to_buffer(buffer, Rect16(0, buff_rows_to_draw - 1, 1, 1), rect.Width(), flag & MIC_ALT_CL_BOT_LEFT ? secondary_color : back);
            }
            if (flag & MIC_BOT_RIGHT) {
                store_to_buffer(buffer, Rect16(rect.Width() - 1, buff_rows_to_draw - 1, 1, 1), rect.Width(), flag & MIC_ALT_CL_BOT_RIGHT ? secondary_color : back);
            }
        }
        display_ex_draw_from_buffer({ uint16_t(rect.Left()), uint16_t(rect.Top() + i * buffROWS) }, rect.Width(), buff_rows_to_draw);
        h_left -= buff_rows_to_draw;
    }

    display_ex_return_buffer();
}

/// Draws rounded rectangle with parametric corner radius
/// flag parameter determines which corners will be drawn rounded

void display_ex_draw_rounded_rect(Rect16 rect, color_t back, color_t front, uint8_t cor_rad, uint8_t flag, color_t secondary_color) {
    if (cor_rad == 0) {
        // front color is color of rext
        // back is not used (would be drawn behind corners)
        display_ex_fill_rect(rect, front);
        return;
    }

    if (rect.Width() <= 0 || rect.Height() <= 0 || !DisplayClip().Contain(rect)) {
        return;
    }

    uint16_t buff_rows = buffROWS;

    uint16_t div = rect.Height() / buff_rows;
    bool carry = true;
    if (div) {
        carry = rect.Height() % buff_rows != 0; // ILI9488 display buffer has only 8 rows, we need to separate
    }
    uint16_t h_left = rect.Height();

    if (cor_rad == 1) {
        // If corner radius is 1 px, we will use simpler and more effective process
        display_ex_draw_rounded_rect_rad1(rect, back, front, flag, (carry ? div + 1 : div), secondary_color);
        return;
    }

    uint8_t *buffer = display_ex_borrow_buffer();

    for (int part = 0; part < (carry ? div + 1 : div); part++) { // seperated parts
        // clear buffer
        uint16_t buff_rows_to_draw = (h_left < buff_rows ? h_left : buff_rows);
        store_to_buffer(buffer, Rect16(0, 0, rect.Width(), buff_rows_to_draw), rect.Width(), front); // We paint whole rect with front color and then paint the round edge's complement in back color TO AVOID FLICKERING

        // cycle trough buffer rows
        for (int i = 0; i < buff_rows_to_draw; i++) {
            uint16_t curr_row = part * buff_rows + i;

            // draw top edges
            if (curr_row < cor_rad) {
                int cnt = 0; // number of pixels that will be drawn (in one line)
                for (int col = 0; col < cor_rad; col++) {
                    if (col * col + (cor_rad - curr_row - 1) * (cor_rad - curr_row - 1) < cor_rad * cor_rad) {
                        cnt++;
                    }
                }

                if (cnt > 0) {
                    // top right corner
                    if (flag & MIC_TOP_RIGHT) {
                        store_to_buffer(buffer, Rect16(rect.Width() - cor_rad + cnt, i, (cor_rad - cnt), 1), rect.Width(), flag & MIC_ALT_CL_TOP_RIGHT ? secondary_color : back);
                    }
                    // top left corner
                    if (flag & MIC_TOP_LEFT) {
                        store_to_buffer(buffer, Rect16(0, i, (cor_rad - cnt), 1), rect.Width(), flag & MIC_ALT_CL_TOP_LEFT ? secondary_color : back);
                    }
                }

                // draw bottom edges
            } else if (curr_row >= rect.Height() - cor_rad) {
                int cnt = 0; // number of pixels that will be drawn (in one line)
                for (int col = 0; col < cor_rad; col++) {
                    if (col * col + (curr_row - (rect.Height() - cor_rad)) * (curr_row - (rect.Height() - cor_rad)) < cor_rad * cor_rad) {
                        cnt++;
                    }
                }

                if (cnt > 0) {
                    // bottom right corner
                    if (flag & MIC_BOT_RIGHT) {
                        store_to_buffer(buffer, Rect16(rect.Width() - cor_rad + cnt, i, (cor_rad - cnt), 1), rect.Width(), flag & MIC_ALT_CL_BOT_RIGHT ? secondary_color : back);
                    }
                    // bottom left corner
                    if (flag & MIC_BOT_LEFT) {
                        store_to_buffer(buffer, Rect16(0, i, (cor_rad - cnt), 1), rect.Width(), flag & MIC_ALT_CL_BOT_LEFT ? secondary_color : back);
                    }
                }
            }
        }
        display_ex_draw_from_buffer({ uint16_t(rect.Left()), uint16_t(rect.Top() + part * buff_rows) }, rect.Width(), buff_rows_to_draw);
        h_left -= buff_rows;
    }

    display_ex_return_buffer();
}

/// Turns the specified pixel to the specified color
void display_ex_set_pixel(point_ui16_t pt, color_t clr) {
    if (!DisplayClip().Contain(pt)) {
        return;
    }
    const uint32_t native_color = color_to_native(clr);
    set_pixel_colorFormatNative(pt.x, pt.y, native_color);
}

void display_ex_set_pixel_displayNativeColor(point_ui16_t pt, uint16_t noClr) {
    if (!DisplayClip().Contain(pt)) {
        return;
    }
    set_pixel_colorFormatNative(pt.x, pt.y, noClr);
}

uint16_t display_ex_get_pixel_displayNativeColor(point_ui16_t pt) {
    if (!DisplayClip().Contain(pt)) {
        return 0;
    }
    return get_pixel_directColor_C(pt.x, pt.y);
}

void display_ex_draw_qoi(point_ui16_t pt, const img::Resource &qoi, color_t back_color, ropfn rop, Rect16 subrect) {
    FILE *file;

    // Use provided file or default resource file
    if (!qoi.file) {
        file = img::get_resource_file();
    } else {
        file = qoi.file;
    }

    if (!file) {
        bsod("Could not get image resource file");
    }

    // Seek to the beginning of the image and draw
    fseek(file, qoi.offset, SEEK_SET);
    draw_qoi_ex_C(file, pt.x, pt.y, back_color, rop, subrect);
}
