#include "json_encode.h"

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

size_t jsonify_str_buffer(const char *input) {
    return jsonify_str_buffer_len(input, strlen(input));
}

size_t jsonify_str_buffer_len(const char *input, size_t len) {
    size_t extra = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '\0') {
            extra += 5;
        } else if (get_special(input[i])) {
            extra++;
        }
    }

    return extra > 0 ? len + extra + 1 : 0;
}

void jsonify_str(const char *input, char *output) {
    return jsonify_str_len(input, strlen(input), output);
}

void jsonify_str_len(const char *input, size_t len, char *output) {
    for (size_t i = 0; i < len; i++) {
        const char sp = get_special(input[i]);
        if (sp) {
            *output++ = '\\';
            *output++ = sp;
        } else if (input[i] == '\0') {
            memcpy(output, "\\u0000", 6);
            output += 6;
        } else {
            *output++ = input[i];
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

size_t unescape_json_i(char *in, size_t size) {
    char *write = in;
    char *read = in;
    const char *end = &in[size];
    while (read < end) {
        if (read + 5 < end && strncmp(read, "\\u0000", 6) == 0) {
            *write = '\0';
            write++;
            read += 6;
            size -= 5;
        } else if (*read == '\\') {
            // to prevent escaping if the \ is the last char we should escape
            // otherwise we would touch the input after size
            if (read + 1 == end) {
                *write++ = *read++;
                break;
            }
            bool escaped = false;
            for (size_t j = 0; j < sizeof special_chars / sizeof *special_chars; j++) {
                if (*(read + 1) == special_chars[j].escape) {
                    escaped = true;
                    *write = special_chars[j].input;
                    write++;
                    read += 2;
                    size--;
                    break;
                }
            }
            if (!escaped) {
                *write++ = *read++;
            }
        } else {
            *write++ = *read++;
        }
    }
    return size;
}

static constexpr char json_escape_symbol = '~';

bool json_escape_bytes(const char *input, char *output, size_t max_output_len) {
    const char *read = input;
    char *write = output;
    char *write_end = output + max_output_len;

    char ch;
    while ((ch = *(read++)) != '\0') {
        // Standard, character -> continue
        if (static_cast<unsigned char>(ch) >= 32 && static_cast<unsigned char>(ch) < 127 && ch != json_escape_symbol && ch != '"') {
            if (write + 1 > write_end) {
                return false;
            }

            *(write++) = ch;
        }

        // Otherwise create an escape sequence
        else {
            // We need 3 chars for the sequence
            if (write + 3 > write_end) {
                return false;
            }

            *(write++) = json_escape_symbol;
            *(write++) = 'a' + (ch & 0xf);
            *(write++) = 'a' + ((ch >> 4) & 0xf);
        }
    }

    // Write terminating \0
    {
        if (write + 1 > write_end) {
            return false;
        }
        *(write++) = '\0';
    }

    return true;
}

bool json_unescape_bytes(const char *input, char *output) {
    const char *read = input;
    char *write = output;

    char ch;
    while ((ch = *(read++)) != '\0') {
        // Escape sequence - decode
        if (ch == json_escape_symbol) {
            const char ch1 = *(read++);
            if (ch1 < 'a' || ch1 >= 'a' + 16) {
                return false;
            }

            const char ch2 = *(read++);
            if (ch2 < 'a' || ch2 >= 'a' + 16) {
                return false;
            }

            *(write++) = (ch1 - 'a') | ((ch2 - 'a') << 4);
        }

        // Not an escape sequence - simply copy
        else {
            *(write++) = ch;
        }
    }

    // Write terminating \0
    *(write++) = '\0';

    return true;
}
