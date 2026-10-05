/// @file
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

struct UUID {
    static constexpr size_t size_bytes = 16;

    // One byte -> 2 hex chars, plus 4 "-" separators
    static constexpr size_t string_length = size_bytes * 2 + 4;

    using Bytes = std::array<std::byte, size_bytes>;
    Bytes bytes;

    /// @returns all-zeroes UUID
    [[nodiscard]] consteval static UUID nil() {
        return UUID { .bytes {} };
    }

    [[nodiscard]] static constexpr UUID from_bytes(std::span<const std::byte, size_bytes> bytes) {
        UUID result;
        std::ranges::copy(bytes, result.bytes.begin());
        return result;
    }

    // To use, you need to include uuid_constexpr.hpp
    [[nodiscard]] static constexpr std::optional<UUID> from_string_constexpr(std::string_view str);

    [[nodiscard]] static std::optional<UUID> from_string(std::string_view str);

    [[nodiscard]] std::string_view to_string(std::span<char> buffer) const;

    /// Constructs a UUIDv5 from a SHA1 hash
    /// @param sha1_hash must be SHA-1 over the namespace UUID bytes (network order) followed by the name bytes.
    [[nodiscard]] static UUID make_v5_from_sha1(std::array<std::byte, 20> sha1_hash);

    template <typename F>
    [[nodiscard]] static UUID make_v5(const UUID &ns, std::span<const std::byte> name, F &&sha1) {
        return make_v5_from_sha1(std::forward<F>(sha1)(ns.bytes, name));
    }

    [[nodiscard]] constexpr bool operator==(const UUID &) const = default;
};
