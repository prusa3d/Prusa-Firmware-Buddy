#pragma once

#include <span>
#include <expected>
#include <type_traits>
#include <concepts>

#include "gcode_basic_parser.hpp"

/// Currently, this is just a nicer wrapper over the Marlin parser.
/// In the future, it will be a proper, encapsulated class
class GCodeParser2 : public GCodeBasicParser {

public:
    enum class OptionError {
        /// The option is not present at all
        not_present,

        /// The option is present, but parsing to the specified type failed
        /// Returning this value is accompanied by calling \p error_callback
        parse_error,
    };

    /// Used for "store_option" functions.
    /// On success, the option value is stored in the \p target reference that the function takes and \p void is returned.
    /// On error, the \p target remains unchanged and an appropriate \p OptionError is returned.
    using StoreOptionResult = std::expected<void, OptionError>;

    using LargestInteger = int32_t;

    static constexpr char first_option_letter = 'A';
    static constexpr char last_option_letter = 'Z';

public:
    // Inherit parent constructors
    using GCodeBasicParser::GCodeBasicParser;

    [[nodiscard]] bool parse(const std::string_view &gcode) override;

public:
    /// \returns Whether the \param option is present in the gcode (does not necessarily have to have a value)
    [[nodiscard]] bool has_option(char option) const {
        return option >= first_option_letter && option <= last_option_letter && option_positions[option - first_option_letter] != 0;
    }

    /// Tries to parse option \param key as type \param T.
    ///  \returns the parsed value on success or \p OptionError if the option is not present or parsing fails.
    template <typename T, typename... Args>
    [[nodiscard]] std::expected<T, OptionError> option(char key, Args &&...args) const {
        T val {};
        const auto result = store_option(key, val, std::forward<Args>(args)...);
        if (!result) {
            return std::unexpected(result.error());
        }

        return val;
    }

    /// \returns `(value, key)` for the first key from \param keys for which parsing succeeds as \param T
    template <typename T, typename... Args>
    [[nodiscard]] std::optional<std::pair<T, char>> option_multikey(std::initializer_list<char> keys, Args &&...args) const {
        T val {};
        for (const char key : keys) {
            if (store_option(key, val, args...)) {
                return { { val, key } };
            }
        }

        return std::nullopt;
    }

    /**
     * !!! General structure of store_option functions:
     * - First parameter is always "char key"
     * - Second parameter is always reference to a target variable, where the result is stored
     * - Must return StoreOptionResult
     * - If the result is an error, \p target must not be changed
     * - Must be const
     */

    /// Save value of option \param key into \param target.
    /// Uses \param buffer as a buffer to store the result (target is a subset of the buffer)
    /// The value is stripped from trailing and leading whitespace.
    /// If the value was quoted (G1 T"some \' str"), it is unquoted and unescaped (target is set to "some ' str")
    StoreOptionResult store_option(char key, std::string_view &target, std::span<char> buffer) const;

    /// Parses a bool option.
    /// Present option \param key with no value is considered as \p true.
    StoreOptionResult store_option(char key, bool &target) const;

    /// Parses an integer.
    /// The parsed value must be within \param min_value and \param max_value (both inclusive), otherwise an error is returned.
    /// \param target is changed only on success.
    StoreOptionResult store_option(char key, LargestInteger &target, LargestInteger min_value = std::numeric_limits<LargestInteger>::min(), LargestInteger max_value = std::numeric_limits<LargestInteger>::max()) const;

    /// Convenience functions for smaller integers
    template <std::integral T>
        requires(!std::is_same_v<T, bool> && !std::is_same_v<T, LargestInteger>)
    StoreOptionResult store_option(char key, T &target, T min_value = std::numeric_limits<T>::min(), T max_value = std::numeric_limits<T>::max()) const {
        LargestInteger val {};
        // There is an explicit overload for LargestInteger, so this does not infinitely recurse
        const auto result = store_option(key, val, static_cast<LargestInteger>(min_value), static_cast<LargestInteger>(max_value));
        if (!result) {
            return result;
        }

        target = val;
        return {};
    }

    /// Parses an enum
    template <typename T>
        requires(std::is_enum_v<T>)
    StoreOptionResult store_option(char key, T &target, T enum_count) const {
        using TB = std::underlying_type_t<T>;
        return store_option<TB>(key, reinterpret_cast<TB &>(target), static_cast<TB>(0), static_cast<TB>(enum_count) - 1);
    }

    /// Parses a float
    StoreOptionResult store_option(char key, float &target, float min_value = -std::numeric_limits<float>::infinity(), float max_value = std::numeric_limits<float>::infinity()) const;

protected:
    /// Parses an option value
    /// \param p is expected to be positioned on the option value position (see \p option_positions)
    /// If \param accumulator is provided, copies the option value to it, in a parsed manner:
    /// - Non-quoted values are copied as-is
    /// - Quoted values are copied without the quotes and unescaped (\" -> ")
    /// - The accumulated string is null-terminated
    std::optional<std::string_view> parse_option_value(GCodeParserHelper &p, std::span<char> accumulator = {}) const;

    /// Note: the first argument of this function is actually the implicit \p this, so the 2, 3 in the \p __attribute__ are correct
    void report_option_error(char key, const char *msg, ...) const __attribute__((format(printf, 3, 4)));

protected:
    /// Stores position of the first character after the option letter within the \p gcode, if the option is present.
    /// Applies for options that don't have a value, too.
    // {} ensures default initialization to 0
    std::array<uint8_t, last_option_letter - first_option_letter + 1> option_positions {};
};
