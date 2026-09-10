#include <catch2/catch_test_macros.hpp>

#include <lfn.h>
#include "ourPosix.hpp"

#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace {

constexpr char sentinel = 0x5a;

// Buffer larger than path_buffer_size, with the tail beyond path_buffer_size
// primed with a sentinel byte, to detect any write past the declared size.
std::vector<char> make_buffer(const char *initial, size_t physical_size) {
    std::vector<char> buf(physical_size, sentinel);
    strcpy(buf.data(), initial);
    return buf;
}

bool sentinel_intact(const std::vector<char> &buf, size_t path_buffer_size) {
    for (size_t i = path_buffer_size; i < buf.size(); i++) {
        if (buf[i] != sentinel) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST_CASE("get_SFN_path: short name shorter than long name", "[lfn]") {
    testFiles0 = { { "longname12.gco", 0, false } };

    const size_t path_buffer_size = 64;
    std::vector<char> buf = make_buffer("/usb/d/longname12.gco", path_buffer_size + 16);

    get_SFN_path(std::span<char>(buf.data(), path_buffer_size));

    CHECK(std::string(buf.data()) == "/usb/d/longna~00.GCO");
    CHECK(sentinel_intact(buf, path_buffer_size));
}

TEST_CASE("get_SFN_path: short name longer than long name is not copied", "[lfn]") {
    // 23 bytes, longer than the "b" long name and than the buffer can hold.
    testFiles0 = { { "b", 0, false, std::string(23, 'A') } };

    const size_t path_buffer_size = 30;
    std::vector<char> buf = make_buffer("/usb/d/b", path_buffer_size + 20);

    get_SFN_path(std::span<char>(buf.data(), path_buffer_size));

    // Does not fit (7 + 23 + 1 > 30) -> left intact, no overflow.
    CHECK(std::string(buf.data()) == "/usb/d/b");
    CHECK(sentinel_intact(buf, path_buffer_size));
}

TEST_CASE("get_SFN_path: equal length names are still substituted", "[lfn]") {
    testFiles0 = { { "AAAA", 0, false, std::string("BBBB") } };

    const size_t path_buffer_size = 64;
    std::vector<char> buf = make_buffer("/usb/d/AAAA", path_buffer_size + 16);

    get_SFN_path(std::span<char>(buf.data(), path_buffer_size));

    CHECK(std::string(buf.data()) == "/usb/d/BBBB");
    CHECK(sentinel_intact(buf, path_buffer_size));
}

TEST_CASE("get_SFN_path: exact fit boundary", "[lfn]") {
    // "/usb/d/" (7) + "ABCDEFGH" (8) + '\0' (1) == 16
    testFiles0 = { { "name", 0, false, std::string("ABCDEFGH") } };

    SECTION("fits exactly") {
        const size_t path_buffer_size = 16;
        std::vector<char> buf = make_buffer("/usb/d/name", path_buffer_size + 16);

        get_SFN_path(std::span<char>(buf.data(), path_buffer_size));

        CHECK(std::string(buf.data()) == "/usb/d/ABCDEFGH");
        CHECK(sentinel_intact(buf, path_buffer_size));
    }

    SECTION("one byte short") {
        const size_t path_buffer_size = 15;
        std::vector<char> buf = make_buffer("/usb/d/name", path_buffer_size + 16);

        get_SFN_path(std::span<char>(buf.data(), path_buffer_size));

        CHECK(std::string(buf.data()) == "/usb/d/name");
        CHECK(sentinel_intact(buf, path_buffer_size));
    }
}

TEST_CASE("get_SFN_path: file not found keeps path intact", "[lfn]") {
    testFiles0 = {};

    const size_t path_buffer_size = 64;
    std::vector<char> buf = make_buffer("/usb/d/missing.gco", path_buffer_size + 16);

    get_SFN_path(std::span<char>(buf.data(), path_buffer_size));

    CHECK(std::string(buf.data()) == "/usb/d/missing.gco");
    CHECK(sentinel_intact(buf, path_buffer_size));
}

TEST_CASE("get_SFN_path: no slash in path keeps path intact", "[lfn]") {
    testFiles0 = {};

    const size_t path_buffer_size = 64;
    std::vector<char> buf = make_buffer("justname.gco", path_buffer_size + 16);

    get_SFN_path(std::span<char>(buf.data(), path_buffer_size));

    CHECK(std::string(buf.data()) == "justname.gco");
    CHECK(sentinel_intact(buf, path_buffer_size));
}
