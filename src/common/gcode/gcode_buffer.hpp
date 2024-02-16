
#pragma once

#include <array>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm> //std::find

/**
 * @brief Helper class to work with line from gcode
 */
class GcodeBuffer {
public:
    // first 80 characters of line is sufficient for most GcodeLine reading purposes rest is discarded
    //
    // Can be reconfigured with the continuations field.
    using Container = std::array<char, 81>;

    Container buffer;
    bool line_complete = true;

    class String {
    public:
        Container::iterator begin;
        Container::iterator end;

        String()
            : begin()
            , end() {}
        String(Container::iterator begin, Container::iterator end)
            : begin(std::move(begin))
            , end(std::move(end)) {}

        void skip_ws();

        /// @brief Skip nonwhitespace characters
        void skip_nws();
        void trim();

        void skip(size_t amount);
        template <class Cond>
        void skip(Cond cond) {
            while (begin != end && cond(*begin)) {
                ++begin;
            }
        }

        char front() const { return *begin; }
        char pop_front() { return begin == end ? '\0' : *begin++; }

        bool is_empty() const { return begin == end; }
        uint32_t get_uint() { return atol(&*begin); }
        float get_float() { return atof(&*begin); };
        String get_string();

        bool operator==(const char *str) const { return std::equal(begin, end, str) && str[end - begin] == '\0' /* safe, after the equal passed */; }

        /// Returns true if the gcode command starts with $str (and is followed by whitespace or string end) and skips the gcode code (plus whitespace).
        /// Returns false and does nothing otherwise.
        bool skip_gcode(const char *gcode_str);

        /// Skips until it finds param definition starting with the $param char. Returns true if found.
        /// On success, the string is positioned after the param char, so it is ready to read the param value.
        /// If the param is not found, returns false and does nothing.
        bool skip_to_param(char param);

        char *c_str() { return &*begin; }
        size_t len() const { return end - begin; }

        typedef std::pair<String, String> parsed_metadata_t;
        /**
         * @brief Parses metadata line of gcode, split name=value to two strings, that are returned in pair
         *
         * @param terminate_value If setting the char after value to \0
         *   (usually, there's at least one char after that in the buffer, but in
         *   the Split continuation mode, that doesn't hold).
         * @return parsed_metadata_t {name, value}, when metadata is not valid, name.begin is nullptr
         */
        parsed_metadata_t parse_metadata(bool terminate_value = true);
    };

    GcodeBuffer();

    String line;
};
