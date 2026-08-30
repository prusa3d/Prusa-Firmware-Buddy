#include "json_encode.h"

#include <stdint.h>
#include <string.h>

struct SpecialChar {
    char input;
    char escape; // The thing after \ in its escaped form.
};

/*
 * JSON doesn't have many characters that *must* be escaped (even though mostly
 * anything *can*). We also assume things will be fine in the face of control
 * characters, invalid unicode, valid unicode... honestly, we are not dealing
 * with the whole spectre of unicode in here. We are a damn printer, not a font
 * rendering library.
 *
 * Also note: This does _not_ contain the \0 in here, because it needs to be
 * escaped as \u0000, which doesn't fit this simplified scheme. Code deals with
 * it separately.
 */
static struct SpecialChar special_chars[] = {
    { '\b', 'b' },
    { '\f', 'f' },
    { '\n', 'n' },
    { '\r', 'r' },
    { '\t', 't' },
    { '"', '"' },
    { '\\', '\\' }
};

/*
 * Returns:
 * - 0 if nothing special (note the above about \0!)
 * - The escape character if it is.
 */
static char get_special(char input) {
    for (size_t i = 0; i < sizeof special_chars / sizeof *special_chars; i++) {
        if (special_chars[i].input == input) {
            return special_chars[i].escape;
        }
    }
    return 0;
}

size_t jsonify_str_buffer(std::string_view input) {
    size_t extra = 0;
    for (const char ch : input) {
        if (ch == '\0') {
            extra += 5;
        } else if (get_special(ch)) {
            extra++;
        }
    }

    return extra > 0 ? input.size() + extra + 1 : 0;
}

void jsonify_str(std::string_view input, char *output) {
    for (const char ch : input) {
        const char sp = get_special(ch);
        if (sp) {
            *output++ = '\\';
            *output++ = sp;
        } else if (ch == '\0') {
            memcpy(output, "\\u0000", 6);
            output += 6;
        } else {
            *output++ = ch;
        }
    }
    *output = '\0';
}

const char *jsonify_bool(bool value) {
    static const char json_true[] = "true";
    static const char json_false[] = "false";
    if (value) {
        return json_true;
    } else {
        return json_false;
    }
}

/// Reads the four hex digits of a \uXXXX escape into *out.
///
/// False if fewer than four bytes remain, or any of them is not hex. jsmn
/// rejects such a string before we ever see it, but unescape_json_i is also
/// called on arbitrary buffers, and on prefixes of a token.
static bool read_hex4(const char *p, const char *end, uint16_t *out) {
    if (end - p < 4) {
        return false;
    }
    uint16_t value = 0;
    for (int i = 0; i < 4; i++) {
        const char c = p[i];
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return false;
        }
        value = (uint16_t)((value << 4) | digit);
    }
    *out = value;
    return true;
}

/// UTF-8 encodes one code point, returning the number of bytes written.
///
/// At most 4, always fewer than the 6 (or 12, for a surrogate pair) input bytes
/// of the escape that produced it - which is what keeps the in-place rewrite in
/// unescape_json_i sound.
static size_t encode_utf8(uint32_t cp, char *out) {
    if (cp < 0x80) {
        // Covers \u0000, which stays a single NUL byte as it always has.
        out[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
}

size_t unescape_json_i(char *in, size_t size) {
    char *write = in;
    char *read = in;
    const char *end = &in[size];

    while (read < end) {
        if (*read != '\\') {
            *write++ = *read++;
            continue;
        }

        // A trailing backslash escapes nothing, and copying it verbatim also
        // keeps us from reading past size.
        if (read + 1 == end) {
            *write++ = *read++;
            break;
        }

        if (read[1] == 'u') {
            uint16_t unit;
            if (read_hex4(read + 2, end, &unit)) {
                uint32_t cp = unit;
                size_t consumed = 6;
                if (unit >= 0xD800 && unit <= 0xDBFF) {
                    // A high surrogate means nothing without a low one after it.
                    uint16_t low;
                    if (end - read >= 12 && read[6] == '\\' && read[7] == 'u'
                        && read_hex4(read + 8, end, &low)
                        && low >= 0xDC00 && low <= 0xDFFF) {
                        cp = 0x10000 + ((uint32_t)(unit - 0xD800) << 10) + (uint32_t)(low - 0xDC00);
                        consumed = 12;
                    } else {
                        cp = 0xFFFD;
                    }
                } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
                    // A low surrogate with no high one before it.
                    cp = 0xFFFD;
                }
                // Unpaired surrogates become U+FFFD rather than falling through
                // to a literal copy. The whole point of decoding these is that a
                // stray backslash downstream is a path separator - FatFs treats
                // one as such - so malformed input must not produce one either.
                write += encode_utf8(cp, write);
                read += consumed;
                continue;
            }
            // Not four hex digits after \u - fall through and copy literally.
        }

        // Deliberately not added to special_chars: that table is shared with
        // get_special(), and putting '/' in it would make us start *emitting*
        // \/ in our own output, which nothing asks for.
        if (read[1] == '/') {
            *write++ = '/';
            read += 2;
            continue;
        }

        bool escaped = false;
        for (size_t j = 0; j < sizeof special_chars / sizeof *special_chars; j++) {
            if (read[1] == special_chars[j].escape) {
                *write++ = special_chars[j].input;
                read += 2;
                escaped = true;
                break;
            }
        }
        // OPEN QUESTION: any other escape is unknown to us and keeps its
        // backslash. jsmn's allowlist rejects all 248 of them before they reach
        // us, so this is unreachable through the parser - but it is the same
        // shape as the bug this decoding was added for, and one parser swap away
        // from mattering. Left as-is rather than changed quietly.
        if (!escaped) {
            *write++ = *read++;
        }
    }

    // The number of bytes written, rather than the running decrements the
    // previous version kept - same value, and the arithmetic no longer has to be
    // re-derived in every branch.
    return (size_t)(write - in);
}
