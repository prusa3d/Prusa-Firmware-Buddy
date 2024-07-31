#pragma once

#include <expected>
#include <type_traits>

/// Currently, this is just a nicer wrapper over the Marlin parser.
/// In the future, it will be a proper, encapsulated class
class GCodeParser2 {

public:
    enum class OptionError {
        /// The option is not present
        not_present,

        /// The option is present, but parsing to the specified type failed
        parse_error,
    };

    template <typename T>
    using OptionResult = std::expected<T, OptionError>;

    struct FromMarlinParser {};
    static constexpr FromMarlinParser from_marlin_parser {};

public:
    GCodeParser2() = delete;

    /// Constructs the parser and takes the data from the
    GCodeParser2(FromMarlinParser);

public:
    /// Tries to parse option \param key as type \param T.
    /// Returns \p std::nullopt if the option is not present or parsing fails.
    template <typename T>
    OptionResult<T> option(char key) const {
        static_assert(false, "Parsing option of this type is not implemented");
    }

    /// Tries to parse option \param key as type \param T.
    /// If the parsing succeeds, sets \param target to the value of the option.
    /// If the option is not present or the parsing fails, the value of \p target remains unchanged.
    /// \returns if the option was stored to \p target
    template <typename T>
    inline bool store_option(char key, T &target) const {
        if (auto v = option<T>(key)) {
            target = *v;
            return true;

        } else {
            return false;
        }
    }
};

// Implemented option overloads, implemented in gcode_parser.cpp:

/// If only \param key is present without any value, it is considered as \p true.
template <>
GCodeParser2::OptionResult<bool> GCodeParser2::option<bool>(char key) const;

template <>
GCodeParser2::OptionResult<float> GCodeParser2::option<float>(char key) const;

template <>
GCodeParser2::OptionResult<uint32_t> GCodeParser2::option<uint32_t>(char key) const;

template <>
GCodeParser2::OptionResult<uint16_t> GCodeParser2::option<uint16_t>(char key) const;

template <>
GCodeParser2::OptionResult<uint8_t> GCodeParser2::option<uint8_t>(char key) const;
