#include "gcode_buffer.hpp"
#include <cctype> // for isspace

GcodeBuffer::GcodeBuffer() {}

void GcodeBuffer::String::skip(size_t amount) {
    begin += std::min(amount, static_cast<size_t>(end - begin));
}

void GcodeBuffer::String::skip_ws() {
    skip([](auto c) -> bool { return isspace(c); });
}

void GcodeBuffer::String::skip_nws() {
    skip([](auto c) -> bool { return !isspace(c); });
}

void GcodeBuffer::String::trim() {
    skip_ws();
    while (begin != end && *(end - 1) == ' ') {
        --end;
    }
}

GcodeBuffer::String GcodeBuffer::String::get_string() {
    skip_ws();
    if (begin != end && *begin == '"') {
        auto quote = std::find(begin + 1, end, '"');
        if (quote != end) {
            return String(begin + 1, quote);
        }
    }
    return String(end, end);
}

bool GcodeBuffer::String::skip_gcode(const char *gcode_str) {
    for (auto it = begin;; ++it, ++gcode_str) {
        if (*gcode_str == '\0') {
            // We matched for example M2, but actual gcode number contiunues (for example M22) => don't match in that case
            if (*it >= '0' && *it <= '9') {
                return false;
            }
            begin = it;
            skip_ws();
            return true;

        } else if (it == end || *it != *gcode_str) {
            return false;
        }
    }
}

bool GcodeBuffer::String::skip_to_param(char param) {
    const auto orig_begin = begin;

    while (!is_empty()) {
        skip_ws();

        if (pop_front() == param) {
            return true;
        }

        skip_nws();
    }

    begin = orig_begin;
    return false;
}

GcodeBuffer::String::parsed_metadata_t GcodeBuffer::String::parse_metadata() {
    parsed_metadata_t result = { GcodeBuffer::String(nullptr, nullptr), GcodeBuffer::String(nullptr, nullptr) };

    skip_ws();
    if (*begin == ';') {
        begin++; // metadata are in format "; name = value", or just "name = value", dependign in if it comes from plaintext gcode or bgcode
    }
    skip_ws();

    auto equal = std::find(begin, end, '=');

    if (equal == end) { // not found
        return result;
    }

    auto name = GcodeBuffer::String(begin, equal);
    name.trim();
    *name.end = 0;
    auto val = GcodeBuffer::String(equal + 1, end);
    val.trim();
    *val.end = 0;
    return { name, val };
}
