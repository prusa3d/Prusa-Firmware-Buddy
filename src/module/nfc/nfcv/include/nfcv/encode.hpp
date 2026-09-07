#pragma once

#include "commands.hpp"
#include "error.hpp"

#include <inplace_vector.hpp>
#include <iso13239/crc.hpp>

#include <expected>
#include <type_traits>
#include <utils/byte_utils.hpp>

namespace nfcv {
using MsgBuilder = stdext::inplace_vector<std::byte, 512>;

Result<void> construct_command(MsgBuilder &builder, const Command &cmd);

/// One of two encoding methods for NFC-V VCD messages.
/// Encodes every bit pair as 1 of 4 valid values.
class Encoder1Of4 {
public:
    static constexpr size_t calculate_message_size(size_t msg_size_in_bytes) {
        return 2 /*SOF + EOF bytes*/ + (msg_size_in_bytes + 2 /*crc*/) * 4 /*stored as bit pairs so 4 per byte*/;
    }

    Encoder1Of4(MsgBuilder &builder);
    ~Encoder1Of4();

    void append_byte(std::byte byte);
    void append_bytes(const Bytes &bytes);

    template <typename T>
    void append_raw(const T &val) {
        append_raw_impl(val);
    }

    void append_crc_and_finalize();

private:
    void append_byte_impl(std::byte byte, bool calculate_crc = true);
    void append_bytes_impl(const Bytes &bytes, bool calculate_crc = true);

    template <typename T>
    void append_raw_impl(const T &val, bool calculate_crc = true) {
        static_assert(std::is_trivial_v<T>);
        static_assert(std::endian::native == std::endian::little);
        append_bytes_impl(std::span { reinterpret_cast<const std::byte *>(&val), sizeof(val) }, calculate_crc);
    }

    MsgBuilder &builder;
    iso13239::CRC crc;
    bool did_finalize;
};
} // namespace nfcv
