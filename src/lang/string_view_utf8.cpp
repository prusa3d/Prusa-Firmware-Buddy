#include "string_view_utf8.hpp"

#include <stdarg.h>
#include <exception>
#include <bitset>

string_view_utf8::Length string_view_utf8::computeNumUtf8Chars() const {
    Length r = 0;
    StringReaderUtf8 reader(*this);
    while (reader.getUtf8Char()) {
        ++r;
    }

    return r;
}

unichar string_view_utf8::getFirstUtf8Char() const {
    StringReaderUtf8 reader(*this);
    return reader.getUtf8Char();
}

size_t string_view_utf8::copyToRAM(char *dst, size_t max_size) const {
    if (max_size == 0) {
        return 0;
    }
    char *dst_start = dst;
    StringReaderUtf8 reader(*this);
    for (size_t i = 0; i < max_size; ++i) {
        *dst = reader.getbyte();
        if (*dst == 0) {
            return dst - dst_start;
        }
        ++dst;
    }

    --dst; // dst pointer is decremented to point at the last character of the buffer
    // terminate string - cut the string in between characters (no leftover prefixes)
    while (dst != dst_start && UTF8_IS_CONT(*dst)) {
        dst--;
    }

    *dst = 0;
    return dst - dst_start;
}

size_t string_view_utf8::copyBytesToRAM(char *dst, size_t buffer_size) const {
    if (buffer_size == 0) {
        return 0;
    }
    StringReaderUtf8 reader(*this);
    char *dst_start = dst;
    for (size_t i = 0; i < buffer_size; ++i) {
        *dst = reader.getbyte();
        if (*dst == 0) {
            return dst - dst_start;
        }
        ++dst;
    }

    // Beware - no multibyte character check!
    // dst pointer is decremented to point at the last character of the buffer
    *(--dst) = 0; // safety termination in case of reaching the end of the buffer
    return dst - dst_start;
}

void FormatBuilder::add_param([[maybe_unused]] const size_t unused, ...) {
    int result_specifiers = -1;
    while (reader.find_format_specifier() && (result_specifiers = reader.read_format_specifier(format_specifier, sizeof(format_specifier))) == 0)
        ; // Find and extract a format specifier, that is not "%%" (ret == 0)

    if (result_specifiers <= 0) {
        // Extraction of format specifier
        std::terminate(); // BSOD
        return;
    }

    va_list args;
    va_start(args, unused);
    const int result_vsnprinf = vsnprintf(params.buffer.data() + target_idx, params.buffer.size_bytes() - target_idx, format_specifier, args);
    va_end(args);
    if (result_vsnprinf < 0) {
        // result_vsnprintf == 0: possible empty string parameter
        std::terminate(); // BSOD
        return;
    }

    // parameter buffered successfully
    target_idx += result_vsnprinf + 1; // + 1 -> null-termination character
}

StringReaderUtf8::StringReaderUtf8(const string_view_utf8 &view)
    : view_(view) //
{
    if (view.type() == Type::formatted_string) {
        // Unwrap formatted string_view;
        parameters = view_.formatted_string_params;
        view_ = parameters->original;
    }
}

static bool is_format_specifier(char c) {
    constexpr char bitset_offset = 32;
    static constexpr std::bitset<96> format_specifiers = []() {
        constexpr const char *chars = "diuoxXfFeEgGaAcspn";
        std::bitset<96> bs;
        for (const char *ch = chars; *ch; ch++) {
            bs.set(*ch - bitset_offset);
        }
        return bs;
    }();
    return c >= bitset_offset && c < (char)format_specifiers.size() + bitset_offset && format_specifiers[c - bitset_offset];
}

static bool is_precision_specifier(char c) {
    constexpr char bitset_offset = 32;
    static constexpr std::bitset<96> precision_specifiers = []() {
        constexpr const char *chars = "0123456789.-+lh";
        std::bitset<96> bs;
        for (const char *ch = chars; *ch; ch++) {
            bs.set(*ch - bitset_offset);
        }
        return bs;
    }();
    return c >= bitset_offset && c < (char)precision_specifiers.size() + bitset_offset && precision_specifiers[c - bitset_offset];
}

bool StringReaderUtf8::find_format_specifier() {
    unichar uch;
    while ((uch = getUtf8Char()) != '\0' && uch != '%')
        ; // Skip all characters before the format specifier including the starting character '%'

    return uch != '\0';
}

int StringReaderUtf8::read_format_specifier(char *buffer, uint8_t buffer_size) {
    char ch = peek();
    advance();
    if (ch == '%') {
        return 0; // escape "%%" found - skip format specifier
    }

    char *buffer_pos = buffer;
    auto copy_to_buff = [&](char ch) {
        if (buffer) {
            if (buffer_pos - buffer < buffer_size) {
                *(buffer_pos++) = ch;
            } else {
                return false;
            }
        }
        return true;
    };

    // Copy '%', pointer was already looking at the next character
    if (!copy_to_buff('%')) {
        return -1;
    }

    while (is_precision_specifier(ch)) {
        if (!copy_to_buff(ch)) {
            return -1;
        }
        ch = peek();
        advance();
    }

    // After optional precision specifiers, we expect a single format specifier and successful copying of the char and termination character
    if (!is_format_specifier(ch) || !copy_to_buff(ch) || !copy_to_buff('\0')) {
        return -1;
    }

    return buffer ? buffer_pos - buffer : 1; // successful skipping returns 1
}

bool StringReaderUtf8::trigger_buffer_switch(uint8_t ch) {
    bool triggered = false;
    if (!switched_to_param_buffer && ch == '%') {
        if (peek() == '%') {
            // escape sequence "%%"
            advance(); // skip '%' character by advancing the pointer and do not switch buffer
        } else {
            // start of a parameter
            read_format_specifier(nullptr, 0); // skip format specifier in the original string_view
            switched_to_param_buffer = triggered = true;
        }
    } else if (switched_to_param_buffer && ch == '\0') {
        // end of a parameter
        switched_to_param_buffer = false; // after this getbyte() extracts from original string_view
        triggered = true;
    }
    return triggered;
}

StringReaderUtf8 &StringReaderUtf8::skip(uint16_t num_of_chars) {
    while (num_of_chars--) {
        getUtf8Char();
    }
    return *this;
}

unichar StringReaderUtf8::getUtf8Char() {
    uint8_t byte = getbyte();

    if (!UTF8_IS_NONASCII(byte)) {
        return byte;
    }
    unichar ord = byte & 0x7F;
    for (unichar mask = 0x40; ord & mask; mask >>= 1) {
        ord &= ~mask;
    }

    while (UTF8_IS_CONT(peek())) {
        byte = getbyte();
        ord = (ord << 6) | (byte & 0x3F);
    }
    return ord;
}

uint8_t StringReaderUtf8::getbyte() {
    uint8_t ch = peek();
    advance();

    if (parameters) {
        while (trigger_buffer_switch(ch)) {
            ch = peek();
            advance();
        }
    }

    return ch;
}

void StringReaderUtf8::advance() {
    if (parameters && switched_to_param_buffer) {
        parameter_idx++;
        return;
    }

    switch (view_.type()) {
    case Type::memory_string:
        view_.memory_ptr++;
        break;

    case Type::file_string:
        view_.file_offset++;
        break;

    case Type::null_string:
    case Type::formatted_string:
        break;
    }
}

uint8_t StringReaderUtf8::peek() const {
    if (parameters && switched_to_param_buffer) {
        return *(parameters->buffer.data() + parameter_idx);
    }

    switch (view_.type()) {
    case Type::memory_string:
        return *view_.memory_ptr;

    case Type::file_string:
        return file_peek();

    case Type::null_string:
    case Type::formatted_string:
        break;
    }

    return '\0'; // Should not happen
}

uint8_t StringReaderUtf8::file_peek() const {
    if (!view_.file) {
        return '\0';
    }

    uint8_t c;
    // sync among multiple reads from the sameMO file
    if (ftell(view_.file) != static_cast<long>(view_.file_offset)) {
        if (fseek(view_.file, view_.file_offset, SEEK_SET) != 0) {
            return '\0';
        }
    }

    if (fread(&c, 1, 1, view_.file) != 1) {
        return '\0';
    }
    return c;
}

FormatBuilder::FormatBuilder(string_view_utf8 str_view, StringViewUtf8ParamBase &params)
    : reader(str_view)
    , params(params) {
    assert(!str_view.isNULLSTR());
    assert(str_view.type() != string_view_utf8::Type::formatted_string);
    assert(params.buffer.size_bytes() > 0);
    params.original = str_view;
}

string_view_utf8 FormatBuilder::finalize() {
    string_view_utf8 result;
    result.file = FORMATTED_STRING_MARKER;
    result.formatted_string_params = &params;
    return result;
}
