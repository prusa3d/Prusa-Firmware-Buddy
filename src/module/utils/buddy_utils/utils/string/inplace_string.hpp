/// @file
#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <bsod/bsod.h>

/// Alternative to std::string with inplace buffer
/// Has the same sizeof() as a std::array.
/// The string is encoded as null-terminated
template <size_t capacity_>
struct InplaceString final {

public:
    constexpr InplaceString() = default;
    constexpr InplaceString(const InplaceString &) = default;

    constexpr InplaceString(std::string_view str) {
        debug_assert(str.size() < capacity_);
        str.copy(data_.data(), capacity_);
        data_[str.size()] = '\0';
    }

    constexpr inline InplaceString(const char *str)
        : InplaceString(std::string_view(str)) {
    }

public:
    static consteval size_t capacity() {
        return capacity_;
    }

    /// @returns number of characters of the string
    constexpr size_t size() const {
        return strnlen(data_.data(), capacity_);
    }

    constexpr inline bool empty() const {
        return data_[0] == '\0';
    }

    constexpr inline const char *data() const {
        return data_.data();
    }

    constexpr inline char *data() {
        return data_.data();
    }

    constexpr inline auto begin() const {
        return data_.begin();
    }

    constexpr inline auto begin() {
        return data_.begin();
    }

    constexpr inline auto end() const {
        return data_.begin() + size();
    }

    constexpr inline auto end() {
        return data_.begin() + size();
    }

public:
    constexpr InplaceString &operator=(const InplaceString &) = default;

    constexpr inline bool operator==(const std::string_view &o) const {
        return operator std::string_view() == o;
    }
    constexpr inline bool operator!=(const std::string_view &o) const {
        return operator std::string_view() != o;
    }

    constexpr operator std::string_view() const {
        return std::string_view { begin(), end() };
    }

public:
    std::array<char, capacity_> data_ { '\0' };
};
