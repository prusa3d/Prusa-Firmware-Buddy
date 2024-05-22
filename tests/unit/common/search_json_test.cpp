#include <search_json.h>

#include <cstring>
#include <catch2/catch.hpp>

using json::Event;
using json::Type;
using std::nullopt;
using std::string;

namespace {

static char JSON[] = "{\"hello\": [1, 2, 3], \"other\":\"stuff\", \"escaped_filename\":\"stupid\\\\_\\\"filename\\t\", \"escaped_object\":{\"name\":\"weird\\\\\\f_chars\\n\"}, \"world\":{\"nested\": nil}}";

const constexpr size_t MAX_TOKENS = 60;

const constexpr Event expected_events[] = {
    Event { 1, Type::Array, "hello", nullopt },
    Event { 2, Type::Primitive, nullopt, "1" },
    Event { 2, Type::Primitive, nullopt, "2" },
    Event { 2, Type::Primitive, nullopt, "3" },
    Event { 1, Type::Pop, nullopt, nullopt },
    Event { 1, Type::String, "other", "stuff" },
    Event { 1, Type::String, "escaped_filename", "stupid\\_\"filename\t" },
    Event { 1, Type::Object, "escaped_object", nullopt },
    Event { 2, Type::String, "name", "weird\\\f_chars\n" },
    Event { 1, Type::Pop, nullopt, nullopt },
    Event { 1, Type::Object, "world", nullopt },
    Event { 2, Type::Primitive, "nested", "nil" },
    Event { 1, Type::Pop, nullopt, nullopt },
};

} // namespace

TEST_CASE("Json structural traversal") {
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    jsmn_init(&parser);

    const auto parse_result = jsmn_parse(&parser, JSON, strlen(JSON), tokens, sizeof tokens / sizeof *tokens);

    REQUIRE(parse_result > 0);

    size_t pos = 0;
    const size_t event_cnt = sizeof expected_events / sizeof *expected_events;

    const bool success = json::search(JSON, tokens, parse_result, [&](const Event &event) {
        INFO("Pos " << pos);
        REQUIRE(pos < event_cnt);
        const Event &expected = expected_events[pos];
        REQUIRE(expected.depth == event.depth);
        REQUIRE(expected.type == event.type);
        // value_or -> so the test report shows something better than {?}
        REQUIRE(expected.key.value_or("") == event.key.value_or(""));
        REQUIRE(expected.value.value_or("") == event.value.value_or(""));
        pos++;
    });

    REQUIRE(success);
    REQUIRE(pos == event_cnt);
}
