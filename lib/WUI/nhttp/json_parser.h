#include <search_json.h>

namespace nhttp::printer {

// We are not completely sure what a token is in the notion of jsmn, besides we
// may need a bit more because it's allowed to put more data in there. It's on
// stack, so likely fine to overshoot a bit.
const constexpr size_t MAX_TOKENS = 30;

enum class JsonParseResult {
    ErrMem,
    ErrReq,
    Ok,
};

template <class Callback>
JsonParseResult parse_command(char *buff, size_t size, Callback &&callback) {
    // For technical reasons in its own function. This releases the used stack
    // before going to talk to marlin (which reportedly uses large stack too).
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
        const bool success = json::search(buff, tokens, parse_result, callback);

        if (success) {
            return JsonParseResult::Ok;
        } else {
            return JsonParseResult::ErrReq;
        }
    }
}

} // namespace nhttp::printer
