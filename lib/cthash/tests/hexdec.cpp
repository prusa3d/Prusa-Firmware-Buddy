#include <cthash/internal/hexdec.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("hexdec basics") {
    constexpr auto v1 = cthash::internal::literal_hexdec_to_binary("");
    REQUIRE((v1 == std::array<std::byte, 0> {}));

    constexpr auto v2 = cthash::internal::literal_hexdec_to_binary("00");
    REQUIRE((v2 == std::array<std::byte, 1> { std::byte { 0x00 } }));

    constexpr auto v3 = cthash::internal::literal_hexdec_to_binary("abcdef01");
    REQUIRE((v3 == std::array<std::byte, 4> { std::byte { 0xab }, std::byte { 0xcd }, std::byte { 0xef }, std::byte { 0x01 } }));
}

template <typename T>
decltype(auto) runtime_pass(T &&val) {
    return val;
}

TEST_CASE("hexdec conversion") {
    auto a = cthash::internal::literal_hexdec_to_binary(runtime_pass(""));
    REQUIRE((a == std::array<std::byte, 0> {}));

    auto b = cthash::internal::literal_hexdec_to_binary(runtime_pass("ab"));
    REQUIRE((b == std::array<std::byte, 1> { std::byte { 0xab } }));
}
