#ifndef CTHASH_INTERNAL_HEXDEC_HPP
#define CTHASH_INTERNAL_HEXDEC_HPP

#include <array>
#include <ostream>
#include <span>
#include <type_traits>

namespace cthash::internal {

consteval auto get_hexdec_table() noexcept {
    std::array<uint8_t, 128> result;

    auto char_to_hexdec = [](char c) {
        if (c >= '0' && c <= '9') {
            return static_cast<uint8_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            return static_cast<uint8_t>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            return static_cast<uint8_t>(c - 'A' + 10);
        } else {
            return static_cast<uint8_t>(0);
        }
    };

    for (int i = 0; i != static_cast<int>(result.size()); ++i) {
        result[static_cast<size_t>(i)] = char_to_hexdec(static_cast<char>(i));
    }

    return result;
}

constexpr auto hexdec_to_value_alphabet = get_hexdec_table();

template <typename CharT>
constexpr auto value_to_hexdec_alphabet = std::array<CharT, 16> { CharT('0'), CharT('1'), CharT('2'), CharT('3'), CharT('4'), CharT('5'), CharT('6'), CharT('7'), CharT('8'), CharT('9'), CharT('a'), CharT('b'), CharT('c'), CharT('d'), CharT('e'), CharT('f') };

struct byte_hexdec_value {
    std::byte val;

    template <typename CharT, typename Traits>
    friend auto &operator<<(std::basic_ostream<CharT, Traits> &os, byte_hexdec_value rhs) {
        return os << value_to_hexdec_alphabet<CharT>[unsigned(rhs.val >> 4u)] << value_to_hexdec_alphabet<CharT>[unsigned(rhs.val) & 0b1111u];
    }
};

template <size_t N, typename CharT>
constexpr auto hexdec_to_binary(std::span<const CharT, N * 2> in) -> std::array<std::byte, N> {
    return [in]<size_t... Idx>(std::index_sequence<Idx...>) {
        return std::array<std::byte, N> { static_cast<std::byte>(hexdec_to_value_alphabet[static_cast<size_t>(in[Idx * 2]) & 0b0111'1111u] << 4u | hexdec_to_value_alphabet[static_cast<size_t>(in[Idx * 2u + 1u]) & 0b0111'1111u])... };
    }
    (std::make_index_sequence<N>());
}

template <typename CharT, size_t N>
requires((N - 1) % 2 == 0) // -1 because of zero terminator in literals
    constexpr auto literal_hexdec_to_binary(const CharT (&in)[N]) -> std::array<std::byte, (N - 1) / 2> {
    return hexdec_to_binary<N / 2>(std::span<const char, N - 1>(in, N - 1));
}

} // namespace cthash::internal

#endif
