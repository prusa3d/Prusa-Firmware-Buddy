#include <gcode_buffer.hpp>
#include <gcode_reader_plaintext.hpp>
#include <unique_file_ptr.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <string>
#include <sys/stat.h>

// A default-constructed String has begin == end == nullptr. No caller passes
// one today, so this pins the contract rather than guarding a live crash.
TEST_CASE("GcodeBuffer::String::parse_metadata: null range doesn't crash", "[gcode_buffer]") {
    GcodeBuffer::String comment;
    const auto [name, val] = comment.parse_metadata();
    CHECK(name.begin == nullptr);
    CHECK(val.begin == nullptr);
}

namespace {

// Creates a temporary plaintext gcode file whose first metadata line is
// exactly `line_len` characters long (excluding the terminating '\n').
unique_file_ptr make_metadata_file(size_t line_len) {
    REQUIRE(line_len >= 2);

    unique_file_ptr file(std::tmpfile());
    REQUIRE(file.get() != nullptr);

    std::string line(line_len, 'a');
    line[0] = ';';
    line[1] = ' ';
    fwrite(line.data(), 1, line.size(), file.get());
    fputc('\n', file.get());

    rewind(file.get());
    return file;
}

} // namespace

// With Continuations::Split, a line whose length is an exact multiple of the
// buffer leaves the next read sitting on the newline, which yields an empty
// line pointing into the buffer. That is how an empty line actually reaches
// parse_metadata.
TEST_CASE("GcodeBuffer::String::parse_metadata: empty line from a split boundary", "[gcode_buffer]") {
    constexpr size_t buffer_size = GcodeBuffer::Container().size();

    auto file = make_metadata_file(buffer_size);
    struct stat st = {};
    REQUIRE(fstat(fileno(file.get()), &st) == 0);

    PlainGcodeReader reader(std::move(file), st);
    REQUIRE(reader.stream_metadata_start());

    GcodeBuffer buffer;
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line.len() == buffer_size);
    REQUIRE(!buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line.is_empty());
    REQUIRE(buffer.line_complete);

    const auto [name, val] = buffer.line.parse_metadata();
    CHECK(name.begin == nullptr);
    CHECK(val.begin == nullptr);
}
