#include <upload_state.h>

#include <catch2/catch.hpp>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

using std::nullopt;
using std::optional;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;

namespace {

#define BOUNDARY    "bOUndAry"
#define CRLF        "\r\n"
#define GCODE_LINE1 "G1 X0 Y0 Z0 E0 F0"

struct Start {
    string filename;
};

struct Finish {
    string tmp_filename;
    string final_filename;
    bool print;
};

struct Log {
    optional<Start> start = nullopt;
    optional<Finish> finish = nullopt;
    vector<string> data;

    void gcode_start(const char *filename) {
        REQUIRE_FALSE(start.has_value());
        REQUIRE_FALSE(finish.has_value());
        start = Start { string(filename) };
    }

    void gcode_data(const char *data, size_t len) {
        REQUIRE(start.has_value());
        REQUIRE_FALSE(finish.has_value());
        this->data.push_back(string(data, len));
    }

    void gcode_finish(const char *tmp_filename, const char *final_filename, bool print) {
        REQUIRE(start.has_value());
        REQUIRE_FALSE(finish.has_value());
        finish = Finish {
            string(tmp_filename),
            string(final_filename),
            print
        };
    }
};

class FakeHandlers : public HttpHandlers {
private:
    static uint32_t start(struct HttpHandlers *self, const char *filename) {
        static_cast<FakeHandlers *>(self)->log.gcode_start(filename);
        return 0;
    }
    static uint32_t data(struct HttpHandlers *self, const char *data, size_t len) {
        static_cast<FakeHandlers *>(self)->log.gcode_data(data, len);
        return 0;
    }
    static uint32_t finish(struct HttpHandlers *self, const char *tmp_filename, const char *final_filename, bool print) {
        static_cast<FakeHandlers *>(self)->log.gcode_finish(tmp_filename, final_filename, print);
        return 0;
    }

public:
    Log log;
    FakeHandlers() {
        // Initialize/zero the C-originating parent
        HttpHandlers *handlers = this;
        memset(handlers, 0, sizeof(*handlers));
        gcode_start = start;
        gcode_data = data;
        gcode_finish = finish;
    }
};

class Test {
public:
    FakeHandlers handlers;
    unique_ptr<Uploader, void (*)(Uploader *)> uploader;
    Test()
        : uploader(uploader_init(BOUNDARY, &handlers), uploader_finish) {}
    void finish() {
        uploader.reset();
    }

    const Log &log() const {
        return handlers.log;
    }

    void feed(const string_view &data) {
        uploader_feed(uploader.get(), data.begin(), data.size());
    }
};

}

// This simply creates and destroyes the parsers and checks that nothing "happens"
TEST_CASE("Unused") {
    Test test;
    test.finish();
    REQUIRE_FALSE(test.log().start.has_value());
    REQUIRE_FALSE(test.log().finish.has_value());
    REQUIRE(test.log().data.empty());
}

/*
 * I know there's a reason to have uniformly formatted code, but these string
 * constants are formatted in blocks that actually correspond to the lines that
 * are sent over the wire. Trying to indent them (in somewhat confused way too)
 * doesn't really help a single bit.
 */
// clang-format off
constexpr const char *const OK_FORM =
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"file\"; filename=\"test.gcode\"" CRLF
CRLF
GCODE_LINE1 CRLF
"--" BOUNDARY "--" CRLF
;

constexpr const char *const DO_PRINT =
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"file\"; filename=\"test.gcode\"" CRLF
CRLF
GCODE_LINE1 CRLF
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"print\"" CRLF
CRLF
"true" CRLF
"--" BOUNDARY "--" CRLF;

constexpr const char *const DO_PRINT_EXTRA_PARTS =
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"extra-file\"; filename=\"not-really\"" CRLF
"Content-Disposition-Trap: name=\"file\"; filename=\"whatever-else\"" CRLF
CRLF
CRLF
"--" BOUNDARY CRLF
"Content-Disposition-Trap: name=\"file\"; filename=\"whatever-else\"" CRLF
"Content-Disposition: form-data; filename=\"test.gcode\"; whatever=\"else\"; name=\"file\"" CRLF
"Content-Disposition-Another-Trap: name=\"file\"; filename=\"whatever-else\"" CRLF
"Disposition: name=\"file\"; filename=\"whatever-else\"" CRLF
CRLF
GCODE_LINE1 CRLF
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"print\"" CRLF
CRLF
"true" CRLF
"--" BOUNDARY "--" CRLF;

constexpr const char *const DONT_PRINT =
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"file\"; filename=\"test.gcode\"" CRLF
CRLF
GCODE_LINE1 CRLF
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"print\"" CRLF
CRLF
"false" CRLF
"--" BOUNDARY "--" CRLF;
// clang-format on

TEST_CASE("One Big Chunk") {
    Test test;
    test.feed(OK_FORM);

    SECTION("With close") {
        test.finish();
    }

    SECTION("Without close") {}

    const auto &log = test.log();
    REQUIRE(log.start.has_value());
    REQUIRE(log.start->filename == "tmp.gcode");
    REQUIRE(log.data.size() == 1);
    REQUIRE(log.data[0] == GCODE_LINE1);
    REQUIRE(log.finish.has_value());
    REQUIRE(log.finish->tmp_filename == "tmp.gcode");
    REQUIRE(log.finish->final_filename == "test.gcode");
    REQUIRE_FALSE(log.finish->print);
}

TEST_CASE("Handling multiple parts") {
    Test test;

    bool expect_start = false;

    SECTION("Do print") {
        expect_start = true;
        test.feed(DO_PRINT);
    }

    SECTION("Ignoring extra stuff") {
        expect_start = true;
        test.feed(DO_PRINT_EXTRA_PARTS);
    }

    SECTION("Don't print") {
        test.feed(DONT_PRINT);
    }

    const auto &log = test.log();
    REQUIRE(log.start.has_value());
    REQUIRE(log.start->filename == "tmp.gcode");
    REQUIRE(log.data.size() == 1);
    REQUIRE(log.data[0] == GCODE_LINE1);
    REQUIRE(log.finish.has_value());
    REQUIRE(log.finish->tmp_filename == "tmp.gcode");
    REQUIRE(log.finish->final_filename == "test.gcode");
    REQUIRE(log.finish->print == expect_start);
}

// Chunk it into small parts. Go as far as single characters to go to the
// extreme and make sure that anything and everything breaks into parts.
TEST_CASE("Chunked") {
    Test test;

    const auto data = string_view(DO_PRINT);
    for (const char *c = data.begin(); c != data.end(); c++) {
        test.feed(string_view(c, 1));
    }

    const auto &log = test.log();
    REQUIRE(log.start.has_value());
    REQUIRE(log.start->filename == "tmp.gcode");
    string result;
    for (const auto &chunk : log.data) {
        result += chunk;
    }
    REQUIRE(result == GCODE_LINE1);
    REQUIRE(log.finish.has_value());
    REQUIRE(log.finish->tmp_filename == "tmp.gcode");
    REQUIRE(log.finish->final_filename == "test.gcode");
    REQUIRE(log.finish->print);
}

// TODO: Malformed
// TODO: Missing filename
// TODO: Incomplete (drop early)
// TODO: Parallel uploads
// TODO: Extra spaces
// TODO: Preamble and epilogue
