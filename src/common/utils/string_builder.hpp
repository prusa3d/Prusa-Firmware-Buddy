/// \file

#include <string_view>
#include <cstddef>
#include <span>

#include <string_view_utf8.hpp>
#include <bsod/bsod.h>

#pragma once

/// Class that allows safely building strings in char buffers.
/// The class ensures that the string is always validly null terminated. Last character in the buffer is always reserved for \0.
/// Use this class instead of snprintf, because it does all the checks.
/// The class does NOT take the OWNERSHIP in the buffer in any way, the buffer has to exist for the whole existence of StringBuilder (but see ArrayStringBuilder)
/// The builder always considers terminating \0, so the actual available size for string is buffer_size - 1
class StringBuilder {

public:
    StringBuilder() = default;

    StringBuilder(std::span<char> span) {
        init(span.data(), span.size());
    }

    /// See StringBuilder::init
    static StringBuilder from_ptr(char *buffer, size_t buffer_size) {
        StringBuilder result;
        result.init(buffer, buffer_size);
        return result;
    }

public:
    /// Initializes string builder on the buffer.
    void init(char *buffer, size_t buffer_size);

public:
    /// Returns false if there is no space left to write further text or if any of the called functions failed
    constexpr inline bool is_ok() const {
        return is_ok_;
    }

    /// See is_ok
    constexpr inline bool is_problem() const {
        return !is_ok_;
    }

    /// Returns number of characters in the buffer (NOT counting the terminating \0 which is always there)
    constexpr inline size_t char_count() const {
        return current_pos_ - buffer_start_;
    }

    /// Returns number of bytes used of the buffer (COUNTING the terminating \0 which is always there)
    constexpr inline size_t byte_count() const {
        return char_count() + 1;
    }

    /// Returns pointer to the first character in the builder buffer
    constexpr inline const char *begin() const {
        return buffer_start_;
    }

    /// Returns pointer behind the last character in the builder buffer (should always point to \0)
    constexpr inline const char *end() const {
        return current_pos_;
    }

public:
    /// Raise BSOD if is_problem(), otherwise do nothing.
    void check() const;

    const char *str() const;

    constexpr inline const char *str_nocheck() const {
        return buffer_start_;
    }

    constexpr inline const uint8_t *str_bytes() const {
        debug_assert(is_ok());
        return reinterpret_cast<const uint8_t *>(buffer_start_);
    }

private:
    StringBuilder &finalize_append();

public:
    StringBuilder &append_char(char ch);

    StringBuilder &append_string(const char *str);

    StringBuilder &append_std_string_view(const std::string_view &view);

    StringBuilder &append_string_view(const string_view_utf8 &str);

    /// Appends text to the builder, using vsnprintf under the hood.
    StringBuilder &__attribute__((format(__printf__, 2, 3)))
    append_printf(const char *fmt, ...);

    /// Appends text to the builder, using vsnprintf under the hood.
    StringBuilder &append_vprintf(const char *fmt, va_list args);

    struct AppendFloatConfig {
        /// Maximum decimal places to print
        uint8_t max_decimal_places = 3;

        /// Always use all max_decimal_places
        bool all_decimal_places : 1 = false;

        /// 0.xxx -> .xxx
        bool skip_zero_before_dot : 1 = false;
    };

    /// Appends a float value
    StringBuilder &append_float(float val, const AppendFloatConfig &config);

public:
    /// Allocates $cnt chars at the end of the string and returns the pointer to them.
    /// Appends \0 at the end of the allocation.
    /// Returns pointer to the allocated string or nullptr on failure
    char *alloc_chars(size_t cnt);

private:
    /// For safety reasons, string builder moving is only for static constructors
    StringBuilder(const StringBuilder &o) = delete;
    StringBuilder(StringBuilder &&) = default;

    StringBuilder &operator=(const StringBuilder &) = delete;
    StringBuilder &operator=(StringBuilder &&) = delete;

private:
    /// Pointer to start of the buffer
    char *buffer_start_ = nullptr;

    /// Pointer after end of the buffer (1 char after the terminating null character)
    char *buffer_end_ = nullptr;

    /// Pointer to the current writable position on the buffer (should always contain terminating null char)
    char *current_pos_ = nullptr;

    /// Error flag, can be set by for example when printf fails
    bool is_ok_ = true;
};

/// StringBuilder bundled together with an array
template <size_t array_size_>
class ArrayStringBuilder : public StringBuilder {

public:
    static constexpr size_t array_size = array_size_;

    inline ArrayStringBuilder()
        : StringBuilder(array) {}

    // The new builder would point to the original array, cannot copy/move
    ArrayStringBuilder(const ArrayStringBuilder &) = delete;
    ArrayStringBuilder(ArrayStringBuilder &&) = delete;

    ArrayStringBuilder &operator=(const ArrayStringBuilder &) = delete;
    ArrayStringBuilder &operator=(ArrayStringBuilder &&) = delete;

private:
    std::array<char, array_size_> array;
};
