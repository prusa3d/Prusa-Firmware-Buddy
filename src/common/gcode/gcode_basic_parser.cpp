#include "gcode_basic_parser.hpp"
#include "gcode_parser_helper.hpp"

#include <array>
#include <type_traits>
#include <cstdlib>
#include <ctype.h>
#include <cstring>

GCodeBasicParser::GCodeBasicParser(const ErrorCallback &error_callback)
    : error_callback_(error_callback) {}

#ifndef UNITTESTS
    #include <gcode/parser.h>

bool GCodeBasicParser::parse_marlin_command() {
    return parse(parser.command_ptr);
}
#endif

bool GCodeBasicParser::parse(const std::string_view &gcode) {
    data_ = {};
    data_.gcode = gcode;

    GCodeParserHelper p(*this, gcode);

    p.skip_whitespaces();

    // Gcode can start with Nxx denoting "line number"
    if (p.ch() == 'N') {
        p.advance();
        const auto line_number = p.parse_integer<LineNumber>();
        if (!line_number.has_value()) {
            return false; // Error already reported in parse_number
        }
        data_.line_number = line_number;

        p.skip_whitespaces();
    }

    // Parse command
    {
        // Letter
        if (p.ch() < 'A' || p.ch() > 'Z') {
            p.report_error("Expected command letter");
            return false;
        }
        data_.command.letter = p.ch();
        p.advance();

        // Codenum
        const auto codenum = p.parse_integer<GCodeCommand::Codenum>();
        if (!codenum.has_value()) {
            return false; // Error already reported in parse_number
        }
        data_.command.codenum = *codenum;

        // Optional subcode
        if (p.ch() == '.') {
            p.advance();
            const auto subcode = p.parse_integer<GCodeCommand::Subcode>();
            if (!subcode.has_value()) {
                return false; // Error already reported in parse_number
            }
            data_.command.subcode = subcode;
        }
    }

    p.skip_whitespaces();

    // Determine/skip body
    {
        const char *body_start = p.ch_ptr();
        const char *body_end = body_start;

        // Accept the body
        // If we hit a comment (;) or checksum (*) -> there will be no more body characters definitely
        while (p.ch() != ';' && p.ch() != '*' && p.ch() != '\0') {
            // If we hit a non-space character, we extend the body.
            // This results in the trailing whitespaces not being included
            if (!isspace(p.ch())) {
                body_end = p.ch_ptr() + 1;
            }

            p.advance();
        }

        data_.body = std::string_view(body_start, body_end);
    }

    data_.is_ok = true;
    return true;
}
