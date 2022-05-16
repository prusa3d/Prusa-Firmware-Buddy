
#include <string_view>
#define JSMN_HEADER
#include <jsmn.h>

namespace nhttp::printer {

/*
 * We are not completely sure what a token is in the notion of jsmn, besides we
 * may need a bit more because it's allowed to put more data in there. It's on
 * stack, so likely fine to overshoot a bit.
 */
const constexpr size_t MAX_TOKENS = 30;

template <class Callback>
bool search_json(const char *input, jsmntok_t *tokens, size_t cnt, Callback &&callback) {
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

enum class JsonParseResult {
    ErrMem,
    ErrReq,
    Ok,
};

template <class Callback>
JsonParseResult parse_command(const char *buff, size_t size, Callback &&callback) {
    /*
         * For technical reasons in its own function. This releases the used stack
         * before going to talk to marlin (which reportedly uses large stack too).
         */
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    jsmn_init(&parser);
    const auto parse_result = jsmn_parse(&parser, buff, size, tokens, sizeof tokens / sizeof *tokens);

    if (parse_result < 0) {
        if (parse_result == JSMN_ERROR_NOMEM) {
            // Too few tokens, give up.
            return JsonParseResult::ErrMem;
        } else {
            // Something else is wrong...
            return JsonParseResult::ErrReq;
        }
    } else {
        const bool success = search_json(buff, tokens, parse_result, callback);

        if (success) {
            return JsonParseResult::Ok;
        } else {
            return JsonParseResult::ErrReq;
        }
    }
}

}
