#pragma once

#include <string_view>
#include <optional>
#include <cstdint>
#include <limits>
#include <cstdarg>

#include <inplace_function.hpp>

struct GCodeCommand {
    using Codenum = uint16_t;
    using Subcode = uint8_t;

    /// "G123.1" -> 'G'
    char letter = '\0';

    /// "G123.1" -> 123
    Codenum codenum = 0;

    /// "G123.1" -> 1
    std::optional<Subcode> subcode;

    bool operator==(const GCodeCommand &) const = default;
    bool operator!=(const GCodeCommand &) const = default;
};

class GCodeParserHelper;

/// Basic GCode parser - only decodes the codenum and splits the body
class GCodeBasicParser {
    friend class GCodeParserHelper;

public:
    using LineNumber = int;

    struct ErrorCallbackArgs {
        const GCodeBasicParser &parser;

        /// Position of the error within the
        size_t position;

        /// Message, to be formatted with printf-style function
        const char *message = nullptr;
    };

    /// Callback function for when error happens.
    /// \param args are variadic args for \p message formatting
    using ErrorCallback = stdext::inplace_function<void(const ErrorCallbackArgs &data, va_list args)>;

public:
    explicit GCodeBasicParser() = default;

    explicit GCodeBasicParser(const ErrorCallback &error_callback);

    [[nodiscard]] virtual bool parse(const std::string_view &gcode);

#ifndef UNITTESTS
    /// Parses the current command marlin server is processing.
    /// !!! To be called only from within the gcode functions.
    [[nodiscard]] bool parse_marlin_command();
#endif

    inline const auto &error_callback() const {
        return error_callback_;
    }
    inline void set_error_callback(const ErrorCallback &set) {
        error_callback_ = set;
    }

    /// \returns the whole gcode
    inline const std::string_view &gcode() const {
        return data_.gcode;
    }

    inline const GCodeCommand &command() const {
        return data_.command;
    }

    /// \returns body of the gcode (the parameters/string), stripped of the gcodecode
    /// For example "G123.1 X5 Y5" -> "X5 Y5"
    /// This is useful for gcodes like M117 "Set Message", where the whole body is an argument.
    inline const std::string_view &body() const {
        return data_.body;
    }

    /// \returns line number of the gcode (specified by "Nxxx" at the beginning of the line), if present
    inline std::optional<LineNumber> line_number() const {
        return data_.line_number;
    }

protected:
    ErrorCallback error_callback_;

    struct {
        std::string_view gcode;

        /// Nxx before the Gcode denotes line number - used for checking of continuity on serial protocols
        std::optional<LineNumber> line_number;

        GCodeCommand command;

        /// "G123.1 X5 Y5" -> "X5 Y5"
        std::string_view body;
    } data_;
};
