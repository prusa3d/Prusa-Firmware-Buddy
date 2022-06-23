#include <string_view>
#include <optional>

#define JSMN_HEADER
#include <jsmn.h>

namespace json {

/// The type of an event/value in json.
enum class Type {
    Object,
    Array,
    String,
    /// Acts in many ways the same as string except that it is not a string.
    ///
    /// Note that jsmn doesn't tell us further info, so there's no distinction
    /// (at least not yet) if it shall be float, int, bool, ...
    Primitive,
    /// Not an actual part of the JSON, but this is emited whenever a nested
    /// structural thing is being left (when going out of an object or array).
    /// It is emited for each level (therefore, if there's `}]}`, three such
    /// events are emited).
    ///
    /// The depth is the one corresponding to the event that started it ‒ so
    /// it can be 1, but not 0.
    Pop,
};

/// An event in a JSON "stream".
///
/// Note that this is not "owned", the string_views point to temporary
/// stack-allocated data.
struct Event {
    /// How deep in the structure this thing is.
    ///
    /// The top-level object is not reported.
    /// Anything inside that top-level object is on level 1. Stuff inside an
    /// array or inner object is one level deeper.
    size_t depth;

    Type type;

    /// The name of the field, if any.
    ///
    /// Not present for array items.
    std::optional<std::string_view> key;

    /// The value of the field (or array item). This is not present if the
    /// value is an array or object, only for primitive fields.
    ///
    /// This is not converted in any way. That is, strings are *not* de-escaped
    /// (at least not yet). Ints, floats or bools are not either and are passed
    /// as original string values.
    std::optional<std::string_view> value;
};

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
                Event event {
                    1,
                    Type::String,
                    key,
                    value,
                };
                callback(event);
                i++;
                break;
            }
            case JSMN_PRIMITIVE: {
                std::string_view value(input + val.start, val.end - val.start);
                Event event {
                    1,
                    Type::Primitive,
                    key,
                    value,
                };
                callback(event);
                i++;
                break;
            }
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
