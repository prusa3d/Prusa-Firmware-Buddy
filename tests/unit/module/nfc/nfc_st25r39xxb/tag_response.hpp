#pragma once

#include <iso13239/crc.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace fake {

namespace detail {

    /// Bit-at-a-time writer for the response stream, whose bits run from the low end of each byte up
    class ResponseBits {
    public:
        void push(bool value) {
            if (bit_count_ % 8 == 0) {
                bytes_.push_back(std::byte { 0 });
            }
            if (value) {
                bytes_.back() |= std::byte(1 << (bit_count_ % 8));
            }
            bit_count_++;
        }

        /// Pushes the low \p count bits of \p value, least significant first
        void push_bits(uint8_t value, unsigned count) {
            for (unsigned bit = 0; bit < count; bit++) {
                push(((value >> bit) & 1) != 0);
            }
        }

        std::vector<std::byte> take() {
            return std::move(bytes_);
        }

    private:
        std::vector<std::byte> bytes_;
        size_t bit_count_ = 0;
    };

} // namespace detail

/// Puts \p frame - response flags, payload and CRC, exactly the bytes nfcv::decode yields - into the
/// wire form a tag answers in: an SOF, each bit coded as a pair of bits, and an EOF (ISO 15693 §8).
///
/// The inverse of nfcv::decode. Takes the CRC rather than computing it so that a test can also state
/// a frame whose CRC does not match.
inline std::vector<std::byte> encode_tag_frame(std::span<const uint8_t> frame) {
    static constexpr uint8_t sof = 0x17;
    static constexpr uint8_t eof = 0x1d;

    detail::ResponseBits bits;
    bits.push_bits(sof, 5);
    for (const auto byte : frame) {
        for (unsigned bit = 0; bit < 8; bit++) {
            // A one travels as 10b and a zero as 01b, low bit of the pair first
            const bool set = ((byte >> bit) & 1) != 0;
            bits.push(!set);
            bits.push(set);
        }
    }
    bits.push_bits(eof, 8);
    return bits.take();
}

/// The wire form of a tag answering \p flags and \p payload, with the CRC the tag would append.
inline std::vector<std::byte> encode_tag_response(uint8_t flags, std::span<const uint8_t> payload) {
    std::vector<uint8_t> frame { flags };
    frame.insert(frame.end(), payload.begin(), payload.end());

    iso13239::CRC crc {};
    crc.add_bytes(frame);
    const auto crc_value = crc.get_result();
    frame.push_back(crc_value & 0xff);
    frame.push_back((crc_value >> 8) & 0xff);

    return encode_tag_frame(frame);
}

} // namespace fake
