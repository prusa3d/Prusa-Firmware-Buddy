#ifndef CTHASH_TESTS_INTERNAL_SUPPORT_HPP
#define CTHASH_TESTS_INTERNAL_SUPPORT_HPP

#include <array>
#include <span>
#include <string_view>
#include <cstddef>

template <typename T>
const auto &runtime_pass(const T &val) {
    return val;
}

template <typename T, size_t N>
auto runtime_pass(const std::array<T, N> &val) {
    return std::span<const T>(val.data(), val.size());
}

template <size_t N, typename T = std::byte>
consteval auto array_of(T value) {
    std::array<T, N> output;
    for (T &val : output)
        val = value;
    return output;
}

template <size_t N, typename T = std::byte>
consteval auto array_of_zeros() {
    return array_of<N, T>(T { 0 });
}

template <size_t N>
constexpr auto to_sv(const std::array<char, N> &in) {
    return std::string_view { in.data(), in.size() };
}

template <size_t N>
constexpr auto to_str(const std::array<char, N> &in) {
    return std::string { in.data(), in.size() };
}

#endif
