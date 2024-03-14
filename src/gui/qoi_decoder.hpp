#pragma once

#include <cstdint>
#include <guitypes.hpp>
#include <span>

namespace qoi {

struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class Decoder {
private:
    enum class State : uint8_t {
        initial,
        op_rgb_expect_r,
        op_rgb_expect_g,
        op_rgb_expect_b,
        op_rgba_expect_r,
        op_rgba_expect_g,
        op_rgba_expect_b,
        op_rgba_expect_a,
        op_luma,
        error,
        pixel,
    };
    State state; ///< Current state of QOI decoder

    Pixel px; ///< Remembered pixel value
    int8_t vg; ///< Green difference for luma operation remembered between first and second byte
    uint8_t run; ///< Number of remaining pixels in current run
    Pixel index[64]; ///< Decompression color index

public:
    Decoder()
        : state(State::initial)
        , px { 0, 0, 0, 255 }
        , vg(0)
        , run(0)
        , index {} {}

    static constexpr size_t HEADER_SIZE = 14; ///< QOI header has fixed size [bytes]

    /**
     * @brief Get size of QOI image from header.
     * @param header QOI header
     * @return size of QOI image
     */
    static constexpr size_ui16_t get_image_size(std::span<uint8_t, HEADER_SIZE> header) {
        return {
            static_cast<uint16_t>(
                static_cast<uint16_t>(header[4]) << 24
                | static_cast<uint16_t>(header[5]) << 16
                | static_cast<uint16_t>(header[6]) << 8
                | static_cast<uint16_t>(header[7])),
            static_cast<uint16_t>(
                static_cast<uint16_t>(header[8]) << 24
                | static_cast<uint16_t>(header[9]) << 16
                | static_cast<uint16_t>(header[10]) << 8
                | static_cast<uint16_t>(header[11]))
        };
    }

    /**
     * Push new byte into QOI decoder.
     * Do not push header bytes into decoder.
     * Precondition: !pull_pixel()
     */
    void push_byte(uint8_t byte);

    /**
     * @brief Pull a pixel from QOI decoder.
     * @return pixel when available, returns garbage if not available
     */
    Pixel pull_pixel();

    /// @return true if a pixel is waiting to be pulled
    inline bool has_pixel() const { return state == State::pixel && run; }

    /// @return true if decoder is in error state
    inline bool is_error() const { return state == State::error; }

    /// @return true if decoder is waiting for more data
    inline bool waits_for_data() const { return state != State::pixel && state != State::error; }

private:
    /**
     * @brief Store pixel value in index.
     */
    inline void index_px() {
        index[(px.r * 3 + px.g * 5 + px.b * 7 + px.a * 11) & 0x3f] = px;
    }
};

/// Transform pixel values according raster operations in "raster_opfn_c.h"
namespace transform {

    // Pixel operations taken from "display_math_helper.h" and modified for Pixel format
    Pixel invert(Pixel pixel);
    bool tolerance(Pixel pixel);
    Pixel swapbw(Pixel pixel);
    Pixel desaturate(Pixel pixel);
    Pixel shadow(Pixel pixel);

    /// Apply all rops
    Pixel apply_rop(Pixel pixel, uint8_t rop);
} // namespace transform

} // namespace qoi
