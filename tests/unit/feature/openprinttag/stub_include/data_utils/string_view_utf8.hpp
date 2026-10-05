#pragma once

#include <cstddef>

#define UTF8_IS_NONASCII(ch) ((ch)&0x80)
#define UTF8_IS_CONT(ch)     (((ch)&0xC0) == 0x80)

struct string_view_utf8 {};

struct StringViewUtf8ParamBase {};

template <size_t>
struct StringViewUtf8Parameters : public StringViewUtf8ParamBase {};

struct StringReaderUtf8 {
    string_view_utf8 view;
    char getbyte() const {
        return 0;
    }
};
