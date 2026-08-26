#include "test_files.hpp"

#include <unique_file_ptr.hpp>

#include <core/core.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {

bgcode::core::EResult check(const char *filename) {
    unique_file_ptr f(fopen(filename, "rb"));
    REQUIRE(f.get() != nullptr);
    std::byte crc_buffer[128];
    return bgcode::core::is_valid_binary_gcode(*f.get(), true, crc_buffer, sizeof(crc_buffer));
}

} // namespace

TEST_CASE("is_valid_binary_gcode: plain file", "[is_valid_binary_gcode]") {
    CHECK(check(NEW_BINARY) == bgcode::core::EResult::Success);
}

TEST_CASE("is_valid_binary_gcode: encrypted file", "[is_valid_binary_gcode]") {
    CHECK(check(NEW_ENCRYPTED) == bgcode::core::EResult::Success);
}

// Encrypted for several printers, so it carries more than one key block and
// exercises the loop over them.
TEST_CASE("is_valid_binary_gcode: multiple key blocks", "[is_valid_binary_gcode]") {
    CHECK(check(NEW_ENCRYPTED_MULTI) == bgcode::core::EResult::Success);
}

TEST_CASE("is_valid_binary_gcode: bad CRC", "[is_valid_binary_gcode]") {
    CHECK(check("test_bad_crc_intro.bgcode") == bgcode::core::EResult::InvalidChecksum);
}
