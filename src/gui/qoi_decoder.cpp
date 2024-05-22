#include "qoi_decoder.hpp"
#include <raster_opfn_c.h>
#include <display_math_helper.h>
#include <cstring>

// Loosely based on qoi_decode() by Dominic Szablewski licensed under MIT license
// https://github.com/phoboslab/qoi/blob/f6dffaf1e8170cdd79945a4fb60f6403e447e020/qoi.h#L488

namespace qoi {

void Decoder::push_byte(uint8_t byte) {
    constexpr const uint8_t QOI_OP_RGB = 0xfe; // 11111110
    constexpr const uint8_t QOI_OP_RGBA = 0xff; // 11111111

    constexpr const uint8_t QOI_MASK_2 = 0xc0; // 11000000
    constexpr const uint8_t QOI_OP_INDEX = 0x00; // 00xxxxxx
    constexpr const uint8_t QOI_OP_DIFF = 0x40; // 01xxxxxx
    constexpr const uint8_t QOI_OP_LUMA = 0x80; // 10xxxxxx
    constexpr const uint8_t QOI_OP_RUN = 0xc0; // 11xxxxxx

    switch (state) {
    case State::initial:
        switch (byte) {
        case QOI_OP_RGB:
            state = State::op_rgb_expect_r;
            break;
        case QOI_OP_RGBA:
            state = State::op_rgba_expect_r;
            break;
        default: {
            switch (byte & QOI_MASK_2) {
            case QOI_OP_INDEX:
                px = index[byte & 0x3f];
                run = 1;
                index_px();
                state = State::pixel;
                break;
            case QOI_OP_DIFF:
                px.b += static_cast<int8_t>(byte & 0x03) - 2;
                px.g += static_cast<int8_t>((byte >> 2) & 0x03) - 2;
                px.r += static_cast<int8_t>((byte >> 4) & 0x03) - 2;
                run = 1;
                index_px();
                state = State::pixel;
                break;
            case QOI_OP_LUMA:
                vg = static_cast<int8_t>(byte & 0x3f) - 32;
                state = State::op_luma;
                break;
            case QOI_OP_RUN:
                run = (byte & 0x3f) + 1;
                index_px();
                state = State::pixel;
                break;
            }
            break;
        }
        }
        break;
    case State::op_rgb_expect_r:
        px.r = byte;
        state = State::op_rgb_expect_g;
        break;
    case State::op_rgb_expect_g:
        px.g = byte;
        state = State::op_rgb_expect_b;
        break;
    case State::op_rgb_expect_b:
        px.b = byte;
        run = 1;
        index_px();
        state = State::pixel;
        break;
    case State::op_rgba_expect_r:
        px.r = byte;
        state = State::op_rgba_expect_g;
        break;
    case State::op_rgba_expect_g:
        px.g = byte;
        state = State::op_rgba_expect_b;
        break;
    case State::op_rgba_expect_b:
        px.b = byte;
        state = State::op_rgba_expect_a;
        break;
    case State::op_rgba_expect_a:
        px.a = byte;
        run = 1;
        index_px();
        state = State::pixel;
        break;
    case State::op_luma:
        px.r += vg - 8 + ((byte >> 4) & 0x0f);
        px.g += vg;
        px.b += vg - 8 + (byte & 0x0f);
        run = 1;
        index_px();
        state = State::pixel;
        break;
    case State::pixel:
    case State::error:
    default:
        state = State::error;
        break;
    }
}

Pixel Decoder::pull_pixel() {
    // Optimize for speed, do not check preconditions
    // assert(state == State::pixel && run);

    if (--run == 0) {
        state = State::initial;
    }
    return px;
}

namespace transform {

    Pixel invert(Pixel pixel) {
        pixel.r = 255 - pixel.r;
        pixel.g = 255 - pixel.g;
        pixel.b = 255 - pixel.b;
        return pixel;
    }

    bool tolerance(Pixel pixel) {
        const uint8_t l = (pixel.r + pixel.g + pixel.b) / 3;
        const uint8_t l0 = (l >= SWAPBW_TOLERANCE) ? (l - SWAPBW_TOLERANCE) : 0;
        const uint8_t l1 = (l <= (255 - SWAPBW_TOLERANCE)) ? (l + SWAPBW_TOLERANCE) : 255;

        return ((l0 <= pixel.r) && (pixel.r <= l1) && (l0 <= pixel.g) && (pixel.g <= l1) && (l0 <= pixel.b) && (pixel.b <= l1));
    }

    Pixel swapbw(Pixel pixel) {
        if (tolerance(pixel)) {
            return invert(pixel);
        }
        return pixel;
    }

    Pixel desaturate(Pixel pixel) {
        uint8_t avg = (pixel.r + pixel.g + pixel.b) / 3;
        pixel.r = avg;
        pixel.g = avg;
        pixel.b = avg;
        return pixel;
    }

    Pixel shadow(Pixel pixel) {
        if (tolerance(pixel)) {
            pixel.r /= 2;
            pixel.g /= 2;
            pixel.b /= 2;
        }
        return pixel;
    }

    Pixel apply_rop(Pixel pixel, uint8_t rop) {
        if (rop & ROPFN_INVERT) {
            pixel = invert(pixel);
        }

        if (rop & ROPFN_SWAPBW) {
            pixel = swapbw(pixel);
        }

        if (rop & ROPFN_DESATURATE) {
            pixel = desaturate(pixel);
        }

        if (rop & ROPFN_SHADOW) {
            pixel = shadow(pixel);
        }

        return pixel;
    }
} // namespace transform

} // namespace qoi
