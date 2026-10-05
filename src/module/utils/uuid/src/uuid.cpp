/// @file
#include <uuid/uuid.hpp>
#include <uuid/uuid_constexpr.hpp>

#include <cstdint>

std::optional<UUID> UUID::from_string(std::string_view str) {
    return from_string_constexpr(str);
}

std::string_view UUID::to_string(std::span<char> buffer) const {
    if (buffer.size() < string_length) {
        return {};
    }

    constexpr const char *digits = "0123456789abcdef";

    size_t pos = 0;
    for (size_t i = 0; i < bytes.size(); i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            buffer[pos++] = '-';
        }

        const uint8_t b = std::to_integer<uint8_t>(bytes[i]);
        buffer[pos++] = digits[b >> 4];
        buffer[pos++] = digits[b & 0xf];
    }

    return std::string_view { buffer.data(), pos };
}

UUID UUID::make_v5_from_sha1(std::array<std::byte, 20> sha1_hash) {
    UUID result;
    std::copy_n(sha1_hash.begin(), size_bytes, result.bytes.begin());

    // Version 5 in the high nibble of byte 6
    result.bytes[6] = (result.bytes[6] & std::byte { 0x0f }) | std::byte { 0x50 };

    // RFC 4122/9562 variant (0b10) in the top two bits of byte 8
    result.bytes[8] = (result.bytes[8] & std::byte { 0x3f }) | std::byte { 0x80 };

    return result;
}
