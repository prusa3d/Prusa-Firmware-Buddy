/// @file
#pragma once
#include "uuid.hpp"

#include <cstdint>

constexpr std::optional<UUID> UUID::from_string_constexpr(std::string_view str) {
    UUID result {};
    uint8_t halfbyte_pos = 0;

    for (char c : str) {
        // Ignore dashes, let the input be creative
        if (c == '-') {
            continue;
        }

        if (halfbyte_pos >= size_bytes * 2) {
            return std::nullopt;
        }

        uint8_t halfbyte_val;
        if (c >= '0' && c <= '9') {
            halfbyte_val = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            halfbyte_val = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            halfbyte_val = c - 'A' + 10;
        } else {
            return std::nullopt;
        }

        auto &result_byte = result.bytes[halfbyte_pos / 2];
        result_byte = (result_byte << 4) | std::byte(halfbyte_val);
        halfbyte_pos++;
    }

    if (halfbyte_pos != size_bytes * 2) {
        return std::nullopt;
    }

    return result;
}

consteval UUID operator""_uuid(const char *str, size_t len) {
    return UUID::from_string_constexpr({ str, len }).value();
}
