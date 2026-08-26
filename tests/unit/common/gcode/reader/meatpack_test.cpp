#include <meatpack.h>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

// Feeds bytes one at a time and drains between calls, exactly like
// stream_getc_decode_meatpacked in gcode_reader_binary.cpp does.
std::string decode(const std::initializer_list<uint8_t> bytes) {
    MeatPack mp;
    std::string result;
    for (const uint8_t byte : bytes) {
        mp.handle_rx_char(byte);
        while (mp.has_result_char()) {
            result.push_back(mp.get_result_char());
        }
    }
    return result;
}

} // namespace

TEST_CASE("MeatPack passes characters through with packing disabled", "[MeatPack]") {
    CHECK(decode({ 'G', '1', '\n' }) == "G1\n");
}

TEST_CASE("MeatPack unpacks a packed byte", "[MeatPack]") {
    // 0x10 packs '0' (lookup index 0) and '1' (index 1).
    CHECK(decode({ 0xFF, 0xFF, 0xFB, 0x10 }) == "01");
}

TEST_CASE("MeatPack decode overflow-prone payload", "[MeatPack]") {
    // FF FF FB enables packing, 1F holds a following literal '1', a lone FF is
    // buffered, and 21 unpacks to "12" - four characters queued from a single
    // handle_rx_char call.
    const std::string decoded = decode({ 0xFF, 0xFF, 0xFB, 0x1F, 0xFF, 0x21 });
    CHECK(decoded == std::string { (char)0xFF, '1', '1', '2' });
}

TEST_CASE("MeatPack no-spaces mode replaces space with E", "[MeatPack]") {
    // FF FF FB enables packing, FF FF F7 enables no-spaces mode, and 0xBB
    // packs index 11 (the space slot) in both nybbles.
    CHECK(decode({ 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xF7, 0xBB }) == "EE");
}

TEST_CASE("MeatPack no-spaces mode can be turned back off", "[MeatPack]") {
    // FF FF F6 disables no-spaces again, so the same instance goes back to
    // decoding index 11 as a space.
    CHECK(decode({ 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xF7, 0xBB, 0xFF, 0xFF, 0xF6, 0xBB }) == "EE  ");
}

TEST_CASE("MeatPack reset clears no-spaces mode", "[MeatPack]") {
    // FF FF F9 resets everything, including packing, so packing is enabled
    // again afterwards.
    CHECK(decode({ 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xF7, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xFB, 0xBB }) == "  ");
}

TEST_CASE("MeatPack no-spaces mode does not leak into a fresh instance", "[MeatPack]") {
    // Run one instance with no-spaces mode enabled, exactly as above.
    decode({ 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xF7, 0xBB });

    // A brand-new instance, with packing enabled but no-spaces left disabled,
    // must still decode index 11 as a plain space.
    CHECK(decode({ 0xFF, 0xFF, 0xFB, 0xBB }) == "  ");
}
