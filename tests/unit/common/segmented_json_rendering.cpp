#include <segmented_json.h>
#include <segmented_json_macros.h>

#include <catch2/catch.hpp>
#include <deque>
#include <string>
#include <string_view>
#include <cstring>

using std::deque;
using std::make_tuple;
using std::string;
using std::string_view;
using std::tuple;

using namespace json;

namespace {

class TestJsonRenderer final : public LowLevelJsonRenderer {
private:
    size_t i = 0;

protected:
    virtual JsonResult content(size_t resume_point, JsonOutput &output) override {
        JSON_START;
        JSON_OBJ_START;
        JSON_FIELD_BOOL("hello", true);
        JSON_COMMA;
        JSON_FIELD_STR("world", "stuff\"escaped");

        if (true) {
            // Make sure we support conditions in the middle of the macro magic
            // (to know why it might be a problem, see the expanded code...)
            JSON_COMMA;
            JSON_FIELD_OBJ("sub-something");
            JSON_FIELD_INT("answer", 42);
            JSON_OBJ_END;
        }

        JSON_COMMA;
        JSON_FIELD_ARR("list");
        // Note: We can use for cycles / while cycles inside the JSON block.
        // But make sure the control variables are preserved in the object, not
        // local.
        for (i = 0; i <= 3; i++) {
            if (i > 0) {
                JSON_COMMA;
            }
            JSON_OBJ_START;
            JSON_FIELD_INT("value", i);
            JSON_OBJ_END;
        }
        JSON_ARR_END;

        JSON_OBJ_END;
        JSON_END;
    }
};

class SmallerJson final : public ChunkRenderer {
private:
    deque<string> parts;

public:
    SmallerJson() {
        parts.push_back("\"");
        parts.push_back("hello world");
        parts.push_back("\"");
    }

    virtual tuple<JsonResult, size_t> render(uint8_t *buffer, size_t len) {
        const size_t chunk_len = parts[0].size();
        if (chunk_len > len) {
            return make_tuple(JsonResult::Incomplete, 0);
        } else {
            // Implementation note. This is suboptimal in the way that this
            // might produce more packets/top-level chunks than necessary,
            // because the JSON_CHUNK does _not_ cycle until. It just tries to
            // call it once every time.
            //
            // This is fine for a test, it makes it simpler, but don't just
            // blindly copy-paste this bad practice into a production code
            // O:-).
            memcpy(buffer, parts[0].data(), chunk_len);
            parts.pop_front();
            return make_tuple(parts.size() > 0 ? JsonResult::Incomplete : JsonResult::Complete, chunk_len);
        }
    }
};

class BiggerJson final : public LowLevelJsonRenderer {
private:
    SmallerJson inner;

protected:
    virtual JsonResult content(size_t resume_point, JsonOutput &output) override {
        JSON_START;
        JSON_OBJ_START;
        // The chunk does not contain the field name, we need to provide it separately.
        JSON_FIELD_CHUNK("hello", inner);
        JSON_OBJ_END;
        JSON_END;
    }
};

const constexpr char *const EXPECTED = "{\"hello\":true,\"world\":\"stuff\\\"escaped\",\"sub-something\":{\"answer\":42},\"list\":[{\"value\":0},{\"value\":1},{\"value\":2},{\"value\":3}]}";
const constexpr char *const EXPECTED_WITH_INNER = "{\"hello\":\"hello world\"}";

} // namespace

TEST_CASE("Json Big Buffer - no split") {
    TestJsonRenderer renderer;

    constexpr const size_t BUF_SIZE = 1024;
    uint8_t buff[BUF_SIZE];

    const auto [result, written] = renderer.render(buff, BUF_SIZE);

    REQUIRE(result == JsonResult::Complete);
    const string_view out(reinterpret_cast<char *>(buff), written);
    REQUIRE(out == EXPECTED);
}

TEST_CASE("Json split buffer") {
    TestJsonRenderer renderer;
    size_t increment;

    SECTION("Small") {
        increment = 25;
    }

    SECTION("Mid") {
        increment = 75;
    }

    SECTION("Large") {
        increment = 100;
    }

    string response;
    auto result = JsonResult::Incomplete;

    while (result != JsonResult::Complete) {
        uint8_t buffer[increment];
        const auto [result_partial, written] = renderer.render(buffer, increment);
        REQUIRE(written <= increment);
        REQUIRE(written > 0);
        response += string_view(reinterpret_cast<char *>(buffer), written);
        result = result_partial;
    }

    REQUIRE(response == EXPECTED);
}

TEST_CASE("Json with inner renderer") {
    BiggerJson renderer;

    string response;

    auto result = JsonResult::Incomplete;
    constexpr size_t len = 100;

    while (result != JsonResult::Complete) {
        uint8_t buffer[len];
        const auto [result_partial, written] = renderer.render(buffer, len);
        REQUIRE(written <= len);
        REQUIRE(written > 0);
        response += string_view(reinterpret_cast<char *>(buffer), written);
        result = result_partial;
    }

    REQUIRE(response == EXPECTED_WITH_INNER);
}

// Here we will fit the first write with {. But the second one won't fit at
// all, even with a fresh new empty buffer, so it detects that it can't fit.
TEST_CASE("Json buffer too small") {
    TestJsonRenderer renderer;
    uint8_t buffer[2];
    const auto [result1, written1] = renderer.render(buffer, 2);
    REQUIRE(result1 == JsonResult::Incomplete);
    REQUIRE(written1 == 1);

    const auto [result2, written2] = renderer.render(buffer, 2);
    REQUIRE(result2 == JsonResult::BufferTooSmall);
    REQUIRE(written2 == 0);
}

using ComposedRender = VariantRenderer<EmptyRenderer, TestJsonRenderer, BiggerJson>;

TEST_CASE("Composed JSON - empty") {
    ComposedRender renderer;

    uint8_t buffer[2];
    const auto [result, written] = renderer.render(buffer, 2);
    REQUIRE(result == JsonResult::Complete);
    REQUIRE(written == 0);
}

TEST_CASE("Composed JSON - with content") {
    ComposedRender renderer { TestJsonRenderer() };

    uint8_t buffer[1024];
    const auto [result, written] = renderer.render(buffer, sizeof buffer);
    REQUIRE(result == JsonResult::Complete);
    REQUIRE(string_view(reinterpret_cast<const char *>(buffer), written) == EXPECTED);
}

TEST_CASE("Seq Renderer") {
    PairRenderer<TestJsonRenderer, TestJsonRenderer> renderer { TestJsonRenderer(), TestJsonRenderer() };

    uint8_t buffer[2048];
    const auto [result, written] = renderer.render(buffer, sizeof buffer);
    REQUIRE(result == JsonResult::Complete);
    string exp = EXPECTED;
    exp += EXPECTED;
    REQUIRE(string_view(reinterpret_cast<const char *>(buffer), written) == exp);
}
