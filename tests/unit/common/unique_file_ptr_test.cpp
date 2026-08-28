#include <catch2/catch_test_macros.hpp>

#include <unique_file_ptr.hpp>

#include <array>
#include <cstdio>
#include <unistd.h>

namespace {

// A path in the build directory; the tests remove what they create.
constexpr const char *test_path = "unique_file_ptr_test.bin";

long file_size(const char *path) {
    unique_file_ptr f(fopen(path, "rb"));
    if (!f) {
        return -1;
    }
    if (fseek(f.get(), 0, SEEK_END) != 0) {
        return -1;
    }
    return ftell(f.get());
}

} // namespace

TEST_CASE("fclose_checked reports a successful close", "[unique_file_ptr]") {
    remove(test_path);

    unique_file_ptr f(fopen(test_path, "wb"));
    REQUIRE(f);
    REQUIRE(fwrite("hello", 1, 5, f.get()) == 5);

    CHECK(fclose_checked(std::move(f)));
    // The point of the check is the data, not the return value.
    CHECK(file_size(test_path) == 5);

    remove(test_path);
}

TEST_CASE("fclose_checked reports a failed flush", "[unique_file_ptr]") {
    remove(test_path);

    unique_file_ptr f(fopen(test_path, "wb"));
    REQUIRE(f);
    REQUIRE(fwrite("hello", 1, 5, f.get()) == 5);

    // Pull the descriptor out from under the stream. The five bytes are still
    // sitting in the stdio buffer, so the write only happens when the stream is
    // flushed - and that flush now fails.
    REQUIRE(close(fileno(f.get())) == 0);

    // This is the whole reason the helper exists: the stream still looks
    // healthy at this point, so checking fwrite and ferror - which is what the
    // callers used to do - proves nothing about what reached the filesystem.
    REQUIRE(ferror(f.get()) == 0);

    CHECK_FALSE(fclose_checked(std::move(f)));
    // Nothing was written, despite every pre-close check passing.
    CHECK(file_size(test_path) == 0);

    remove(test_path);
}

TEST_CASE("fclose_checked refuses a file that was never opened", "[unique_file_ptr]") {
    unique_file_ptr f(fopen("/no/such/directory/file.bin", "wb"));
    REQUIRE_FALSE(f);

    CHECK_FALSE(fclose_checked(std::move(f)));
}
