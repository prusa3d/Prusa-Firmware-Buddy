#include <nhttp/segmented_json.h>
#include <nhttp/segmented_json_macros.h>

#include <catch2/catch.hpp>
#include <string>
#include <string_view>

using std::string;
using std::string_view;

using namespace nhttp;

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

const constexpr char *const EXPECTED = "{\"hello\":true,\"world\":\"stuff\\\"escaped\",\"sub-something\":{\"answer\":42},\"list\":[{\"value\":0},{\"value\":1},{\"value\":2},{\"value\":3}]}";

}

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
