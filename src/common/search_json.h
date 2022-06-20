#include <string_view>
#define JSMN_HEADER
#include <jsmn.h>

namespace json {

template <class Callback>
bool search(const char *input, jsmntok_t *tokens, size_t cnt, Callback &&callback) {
    if (cnt == 0) {
        return false;
    }

    if (tokens[0].type != JSMN_OBJECT) {
        return false;
    }

    // #0 is the top-level object.
    for (size_t i = 1; i < cnt; i++) {
        if (tokens[i].type == JSMN_STRING) {
            /*
                 * FIXME: jsmn doesn't decode the strings. We simply hope they
                 * don't contain any escape sequences.
                 */
            std::string_view key(input + tokens[i].start, tokens[i].end - tokens[i].start);
            // Parsing made sure there's another one.
            auto &val = tokens[i + 1];
            switch (val.type) {
            case JSMN_STRING: {
                std::string_view value(input + val.start, val.end - val.start);
                callback(key, value);
                // Fall through to primitive
            }
            case JSMN_PRIMITIVE:
                i++;
                break;
            case JSMN_ARRAY:
            case JSMN_OBJECT:
                /*
                     * FIXME: These are not yet implemented. We need to deal somehow with all the tokens. Options:
                     * * Use the parent links. Nevertheless, it seems enabling
                     *   it for jsmn confuses it and it simply returs fully bogus
                     *   results.
                     * * Understand the structure and traverse it. A lot of work to do.
                     */
            default:
                return false;
            }
        } else {
            // Non-string key...
            return false;
        }
    }

    return true;
}

}
