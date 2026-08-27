#pragma once

#include "raster_opfn.hpp"
#include <cstddef>
#include <cstdint>
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

    [[nodiscard]] static constexpr uint32_t read_be32(std::span<const std::byte, 4> bytes) {
        return std::to_integer<uint32_t>(bytes[0]) << 24
            | std::to_integer<uint32_t>(bytes[1]) << 16
            | std::to_integer<uint32_t>(bytes[2]) << 8
            | std::to_integer<uint32_t>(bytes[3]);
    }

public:
    Decoder()
        : state(State::initial)
        , px { 0, 0, 0, 255 }
        , vg(0)
        , run(0)
        , index {} {}

    static constexpr size_t HEADER_SIZE = 14; ///< QOI header has fixed size [bytes]

    /// Extract image width from QOI header.
    [[nodiscard]] static constexpr uint32_t get_image_width(std::span<const std::byte, HEADER_SIZE> header) {
        return read_be32(header.subspan<4, 4>());
    }

    /// Extract image height from QOI header.
    [[nodiscard]] static constexpr uint32_t get_image_height(std::span<const std::byte, HEADER_SIZE> header) {
        return read_be32(header.subspan<8, 4>());
    }

    /**
     * Push new byte into QOI decoder.
     * Do not push header bytes into decoder.
     * Precondition: !pull_pixel()
     */
    void push(std::byte byte);

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

namespace transform {
    /// Apply all rops
    Pixel apply_rop(Pixel pixel, ropfn rop);
} // namespace transform

} // namespace qoi
