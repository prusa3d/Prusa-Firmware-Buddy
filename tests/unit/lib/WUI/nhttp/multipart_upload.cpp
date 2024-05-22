#include <upload_state.h>

#include <catch2/catch.hpp>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

using http::Status;
using std::get;
using std::make_tuple;
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

struct Finish {
    string final_filename;
    bool print;
};

class Log : public nhttp::printer::UploadHooks {
public:
    optional<Finish> termination = nullopt;
    vector<string> chunks;

    virtual Result data(string_view data) override {
        REQUIRE_FALSE(termination.has_value());
        chunks.push_back(string(data));
        return make_tuple(Status::Ok, nullptr);
    }

    virtual Result check_filename(const char *filename) const override {
        return make_tuple(Status::Ok, nullptr);
    }

    virtual Result finish(const char *final_filename, bool start_print) override {
        REQUIRE_FALSE(termination.has_value());
        termination = Finish {
            string(final_filename),
            start_print
        };
        return make_tuple(Status::Ok, nullptr);
    }
};

class BrokenData : public Log {
public:
    virtual Result data(string_view data) override {
        Log::data(data);
        return make_tuple(Status::IMATeaPot, "Bheeeeee");
    }
};

class Test {
public:
    unique_ptr<Log> log_hooks;
    nhttp::printer::UploadState uploader;
    Test()
        : log_hooks(new Log())
        , uploader(BOUNDARY) {
        uploader.setup(log_hooks.get());
    }

    void change_hooks(Log *l) {
        log_hooks.reset(l);
        uploader.setup(log_hooks.get());
    }

    bool done() {
        return uploader.done();
    }

    const Log &log() const {
        return *log_hooks.get();
    }

    void feed(const string_view &data) {
        uploader.feed(data);
    }

    Status error() {
        return get<0>(uploader.get_error());
    }
};

} // namespace

// This simply creates and destroyes the parsers and checks that nothing "happens"
TEST_CASE("Unused") {
    Test test;
    REQUIRE_FALSE(test.done());
    REQUIRE_FALSE(test.log().termination.has_value());
    REQUIRE(test.log().chunks.empty());
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
"--" BOUNDARY "--" CRLF;

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
// FIXME: Numbers in header names are (wrongly) refused by the multipart-parser.
// "Content-Disposition-2: name=\"file\"; filename=\"whatever-else\"" CRLF
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

constexpr const char *const EXTRA_SPACES =
"--" BOUNDARY CRLF
// FIXME: The multipart parser doesn't accept space before the colon even
// though it probably should.
"Content-Disposition: form-data ; name = \"file\" ; filename =  \"test.gcode\"  " CRLF
CRLF
GCODE_LINE1 CRLF
"--" BOUNDARY "--" CRLF;

constexpr const char *const NO_FILENAME =
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"file\"" CRLF
CRLF
GCODE_LINE1 CRLF
"--" BOUNDARY "--" CRLF;

constexpr const char *const INCOMPLETE =
"--" BOUNDARY CRLF
"Content-Disposition: form-data; name=\"file\"; filename=\"test.gcode\"" CRLF
CRLF
GCODE_LINE1 CRLF;

constexpr const char *const MALFORMED =
"--" BOUNDARY CRLF
"--" BOUNDARY CRLF
": form-data; name=\"file\"; filename=\"test.gcode\"" CRLF
"--" BOUNDARY CRLF
GCODE_LINE1 CRLF;

// clang-format on

TEST_CASE("One Big Chunk") {
    Test test;
    test.feed(OK_FORM);

    REQUIRE(test.error() == Status::Ok);

    const auto &log = test.log();
    REQUIRE(log.chunks.size() == 1);
    REQUIRE(log.chunks[0] == GCODE_LINE1);
    REQUIRE(log.termination.has_value());
    REQUIRE(log.termination->final_filename == "test.gcode");
    REQUIRE_FALSE(log.termination->print);
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
    REQUIRE(log.chunks.size() == 1);
    REQUIRE(log.chunks[0] == GCODE_LINE1);
    REQUIRE(log.termination.has_value());
    REQUIRE(log.termination->final_filename == "test.gcode");
    REQUIRE(log.termination->print == expect_start);
    REQUIRE(test.error() == Status::Ok);
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
    string result;
    for (const auto &chunk : log.chunks) {
        result += chunk;
    }
    REQUIRE(result == GCODE_LINE1);
    REQUIRE(log.termination.has_value());
    REQUIRE(log.termination->final_filename == "test.gcode");
    REQUIRE(log.termination->print);
    REQUIRE(test.error() == Status::Ok);
}

TEST_CASE("Extra spaces") {
    Test test;
    test.feed(EXTRA_SPACES);

    const auto &log = test.log();
    REQUIRE(log.chunks.size() == 1);
    REQUIRE(log.chunks[0] == GCODE_LINE1);
    REQUIRE(log.termination.has_value());
    REQUIRE(log.termination->final_filename == "test.gcode");
    REQUIRE_FALSE(log.termination->print);
    REQUIRE(test.error() == Status::Ok);
}

TEST_CASE("Prolog and epilogue") {
    Test test;

    // FIXME: The multipart parser doesn't properly handle prologues.
    // Testing at least the epilogue.
    const auto all = string() + OK_FORM + "Some epilogue";
    test.feed(all);

    const auto &log = test.log();
    REQUIRE(log.chunks.size() == 1);
    REQUIRE(log.chunks[0] == GCODE_LINE1);
    REQUIRE(log.termination.has_value());
    REQUIRE(log.termination->final_filename == "test.gcode");
    REQUIRE_FALSE(log.termination->print);
    REQUIRE(test.error() == Status::Ok);
}

TEST_CASE("No filename") {
    Test test;
    test.feed(NO_FILENAME);
    REQUIRE(test.error() == Status::BadRequest);
    REQUIRE_FALSE(test.done());

    const auto &log = test.log();
    REQUIRE_FALSE(log.termination.has_value());
    REQUIRE(log.chunks.size() == 0);
}

TEST_CASE("Incomplete upload") {
    Test test;
    test.feed(INCOMPLETE);
    REQUIRE(test.error() == 200);

    const auto &log = test.log();
    REQUIRE(log.chunks.size() == 1);
    REQUIRE(log.chunks[0] == GCODE_LINE1);
    REQUIRE_FALSE(log.termination.has_value());
}

TEST_CASE("Malformed") {
    Test test;
    test.feed(MALFORMED);

    const auto &log = test.log();
    REQUIRE(log.chunks.size() == 0);
    REQUIRE_FALSE(log.termination.has_value());
    REQUIRE(test.error() == Status::BadRequest);
    REQUIRE_FALSE(test.done());
    REQUIRE(!log.termination.has_value());
}

TEST_CASE("Propagate errors from hooks") {
    Test test;
    test.change_hooks(new BrokenData);
    test.feed(OK_FORM);

    REQUIRE(test.error() == Status::IMATeaPot);

    const auto &log = test.log();
    REQUIRE_FALSE(log.termination.has_value());
}

TEST_CASE("Multiple uploads") {
    Test test1;
    Test test2;

    const auto data = string_view(DO_PRINT);
    for (const char *c = data.begin(); c != data.end(); c++) {
        test1.feed(string_view(c, 1));
        test2.feed(string_view(c, 1));
    }

    const auto &log1 = test1.log();
    string result;
    for (const auto &chunk : log1.chunks) {
        result += chunk;
    }
    REQUIRE(result == GCODE_LINE1);
    REQUIRE(log1.termination.has_value());
    REQUIRE(log1.termination->final_filename == "test.gcode");
    REQUIRE(log1.termination->print);

    const auto &log2 = test1.log();
    result.clear();
    for (const auto &chunk : log2.chunks) {
        result += chunk;
    }
    REQUIRE(result == GCODE_LINE1);
    REQUIRE(log2.termination.has_value());
    REQUIRE(log2.termination->final_filename == "test.gcode");
    REQUIRE(log2.termination->print);
    REQUIRE(test2.error() == Status::Ok);
    REQUIRE(test2.error() == Status::Ok);
}
