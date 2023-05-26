#ifndef CTHASH_CONCEPTS_HPP
#define CTHASH_CONCEPTS_HPP

#include <span>

namespace cthash {

template <typename T>
concept one_byte_char = (sizeof(T) == 1u);

template <typename T>
concept byte_like = (sizeof(T) == 1u) && (std::same_as<T, char> || std::same_as<T, unsigned char> || std::same_as<T, char8_t> || std::same_as<T, std::byte> || std::same_as<T, uint8_t> || std::same_as<T, int8_t>);

template <one_byte_char CharT, size_t N>
void string_literal_helper(const CharT (&)[N]);

template <typename T>
concept string_literal = requires(const T &in) //
{
    string_literal_helper(in);
};

template <typename T>
concept convertible_to_byte_span = requires(T &&obj) //
{
    { std::span(obj) };
    requires byte_like<typename decltype(std::span(obj))::value_type>;
    requires !string_literal<T>;
};

} // namespace cthash

#endif
