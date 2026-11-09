#pragma once
#include <string_view>
#include <optional>

#define JSMN_HEADER
#include <jsmn.h>
#include "json_encode.h"

#include <bsod/bsod.h>

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
/// Note that this is not "owned", the string_views point to data inside
/// the JSON buffer passed to search(). The buffer is mutated in place
/// (de-escape + null-termination), so the input buffer must be writable
/// and must remain alive for the duration of the callbacks.
///
/// **Null-termination guarantee:** since the buffer is mutable, both
/// `key` and `value` string_views are null-terminated — the byte at
/// `view.data() + view.size()` is `\0`. Callers may pass `.data()` to
/// C-string APIs or construct `std::string_view(const char*)` safely.
/// (Without this guarantee, the previous byte was whatever JSON syntax
/// followed the token, which silently leaked into anything that read
/// to `\0` — see the commit that added this guarantee for the bug class
/// it fixes.)
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
    /// Strings are de-escaped. Ints, floats or bools are passed
    /// as original string values.
    std::optional<std::string_view> value;
};

namespace impl {

    // Internal recursive traversal implementation.
    //
    // Takes the current token to process and returns the pointer to the next yet
    // unused token. In case of error, returns nullptr.
    template <class Callback>
    jsmntok_t *search_recursive(char *input, jsmntok_t *token, bool emit_self, std::optional<std::string_view> key, size_t depth, Callback &&callback) {
        switch (token->type) {
        case JSMN_OBJECT:
        case JSMN_ARRAY: {
            jsmntok_t *pos = token + 1;
            const bool is_object = token->type == JSMN_OBJECT;
            Event event {
                depth,
                is_object ? Type::Object : Type::Array,
                key,
                std::nullopt,
            };
            if (emit_self) {
                callback(event);
            }
            for (int i = 0; pos && (i < token->size); i++) {
                std::optional<std::string_view> key_tmp = std::nullopt;
                if (is_object) {
                    debug_assert(pos->size == 1);
                    if (pos->type != JSMN_STRING) {
                        return nullptr;
                    }
                    // Overwrite the closing '"' of the key with '\0' so the
                    // resulting string_view is C-string-safe. The byte at
                    // pos->end is the byte AFTER the last char of the key,
                    // which for a JSON string key is always the closing
                    // quote — JSMN has already consumed it, nothing else
                    // needs it.
                    input[pos->end] = '\0';
                    key_tmp = std::string_view(input + pos->start, pos->end - pos->start);
                    pos++;
                }
                pos = search_recursive(input, pos, true, key_tmp, depth + 1, callback);
            }
            if (emit_self) {
                event.type = Type::Pop;
                event.key = std::nullopt;
                callback(event);
            }
            return pos;
        }
        case JSMN_STRING:
        case JSMN_PRIMITIVE: {
            auto new_size = unescape_json_i(input + token->start, token->end - token->start);
            // Null-terminate the value so callers can pass .data() to
            // C-string APIs. The byte at `start + new_size` is safe to
            // overwrite: for strings shorter than the original quoted
            // length it's leftover unescape garbage, for strings of full
            // length it's the closing '"', and for primitives it's the
            // following JSON syntax char (',', '}', ']', or whitespace).
            // In all cases JSMN has already consumed the byte, nothing
            // else needs it.
            input[token->start + new_size] = '\0';
            std::string_view value(input + token->start, new_size);
            Event event {
                depth,
                token->type == JSMN_STRING ? Type::String : Type::Primitive,
                key,
                value,
            };
            callback(event);
            return token + 1;
        }
        default:
            return nullptr;
        }
    }

} // namespace impl

/// Search a JSON (pre-parsed by JSMN).
///
/// It calls the callback for each event ‒ see above.
///
/// Events are for fields, values in arrays, and for structural "traversal".
/// The depth can be used to see on which level (how nested) the event is and
/// look for example only for top-level fields of given names. In particular,
/// it is possible to accep JSON containing additional unknown fields,
/// subfields, etc (and not confuse a sub-sub-sub-field of the same name with
/// the required one on top level).
///
/// The value strings are de-escaped and null-terminated in place. The
/// input buffer is mutated; do not rely on it being unchanged afterwards.
/// See unescape_json_i() and the Event struct doc for details.
///
/// Returns true on success, false on "broken" JSON (mostly if the top-level
/// thing isn't an object). It does expect "structural" validity of the tokens,
/// that is the sizes must be correct - range checking is not performed.
template <class Callback>
bool search(char *input, jsmntok_t *tokens, size_t cnt, Callback &&callback) {
    if (cnt == 0) {
        return false;
    }

    if (tokens[0].type != JSMN_OBJECT) {
        return false;
    }

    return impl::search_recursive(input, tokens, false, std::nullopt, 0, callback) != nullptr;
}

} // namespace json
