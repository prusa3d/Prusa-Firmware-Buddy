#include <stdx/bitset.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

TEST_CASE("bitset storage rounds up to nearest element size", "[bitset]") {
    static_assert(sizeof(stdx::bitset<1, std::uint8_t>) == 1);
    static_assert(sizeof(stdx::bitset<8, std::uint8_t>) == 1);
    static_assert(sizeof(stdx::bitset<9, std::uint8_t>) == 2);

    static_assert(sizeof(stdx::bitset<1, std::uint16_t>) == 2);
    static_assert(sizeof(stdx::bitset<16, std::uint16_t>) == 2);
    static_assert(sizeof(stdx::bitset<17, std::uint16_t>) == 4);
}

TEST_CASE("bitset with implicit storage element type", "[bitset]") {
    static_assert(sizeof(stdx::bitset<1>) == 1);
    static_assert(sizeof(stdx::bitset<8>) == 1);
    static_assert(sizeof(stdx::bitset<9>) == 2);
    static_assert(sizeof(stdx::bitset<16>) == 2);
    static_assert(sizeof(stdx::bitset<17>) == 4);
    static_assert(sizeof(stdx::bitset<32>) == 4);
    static_assert(sizeof(stdx::bitset<33>) == 8);
    static_assert(sizeof(stdx::bitset<64>) == 8);
}

TEMPLATE_TEST_CASE("bitset size", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    static_assert(stdx::bitset<1, TestType>{}.size() == 1);
    static_assert(stdx::bitset<8, TestType>{}.size() == 8);
}

TEMPLATE_TEST_CASE("index operation", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    static_assert(not stdx::bitset<1, TestType>{}[0]);
}

TEMPLATE_TEST_CASE("set single bit", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    auto bs = stdx::bitset<1, TestType>{};
    CHECK(not bs[0]);
    bs.set(0);
    CHECK(bs[0]);
    bs.set(0, false);
    CHECK(not bs[0]);
}

TEMPLATE_TEST_CASE("set all bits", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    auto bs = stdx::bitset<1, TestType>{};
    CHECK(not bs[0]);
    bs.set();
    CHECK(bs[0]);
}

TEMPLATE_TEST_CASE("reset single bit", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    auto bs = stdx::bitset<1, TestType>{1ul};
    CHECK(bs[0]);
    bs.reset(0);
    CHECK(not bs[0]);
}

TEMPLATE_TEST_CASE("reset all bits", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    auto bs = stdx::bitset<1, TestType>{1ul};
    CHECK(bs[0]);
    bs.reset();
    CHECK(not bs[0]);
}

TEMPLATE_TEST_CASE("flip single bit", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    auto bs = stdx::bitset<3, TestType>{0b101ul};
    CHECK(bs[0]);
    CHECK(not bs[1]);
    CHECK(bs[2]);
    bs.flip(0);
    CHECK(not bs[0]);
    CHECK(not bs[1]);
    CHECK(bs[2]);
}

TEMPLATE_TEST_CASE("flip all bits", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    auto bs = stdx::bitset<3, TestType>{0b101ul};
    CHECK(bs[0]);
    CHECK(not bs[1]);
    CHECK(bs[2]);
    bs.flip();
    CHECK(not bs[0]);
    CHECK(bs[1]);
    CHECK(not bs[2]);
}

TEMPLATE_TEST_CASE("construct with a value", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto bs1 = stdx::bitset<1, TestType>{1ul};
    static_assert(bs1[0]);

    constexpr auto bs2 = stdx::bitset<3, TestType>{255ul};
    static_assert(bs2[0]);
    static_assert(bs2[1]);
}

TEMPLATE_TEST_CASE("construct with values for bits", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<1, TestType>{stdx::place_bits, 1, 3, 5};
    static_assert(not bs[0]);
    static_assert(bs[1]);
    static_assert(bs[3]);
    static_assert(bs[5]);
}

TEMPLATE_TEST_CASE("construct with a string_view", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    using namespace std::string_view_literals;
    static_assert(stdx::bitset<4, TestType>{"1010"sv} ==
                  stdx::bitset<4, TestType>{0b1010ul});
}

TEMPLATE_TEST_CASE("construct with a substring", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    using namespace std::string_view_literals;
    static_assert(stdx::bitset<4, TestType>{"XOXOXO"sv, 2, 4, 'X'} ==
                  stdx::bitset<4, TestType>{0b1010ul});
}

TEMPLATE_TEST_CASE("convert to uint64_t", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<3, TestType>{255ul};
    constexpr auto val = bs.to_uint64_t();
    static_assert(std::is_same_v<decltype(val), std::uint64_t const>);
    static_assert(val == 7ul);
}

TEMPLATE_TEST_CASE("all", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs1 =
        stdx::bitset<8, TestType>{std::numeric_limits<TestType>::max()};
    static_assert(bs1.all());

    constexpr auto bs2 = stdx::bitset<9, TestType>{0x1fful};
    static_assert(bs2.all());

    constexpr auto bs3 = stdx::bitset<8, TestType>{0xf7ul};
    static_assert(not bs3.all());
}

TEMPLATE_TEST_CASE("any", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs1 = stdx::bitset<8, TestType>{8ul};
    static_assert(bs1.any());

    constexpr auto bs2 = stdx::bitset<9, TestType>{0xfful};
    static_assert(bs2.any());

    constexpr auto bs3 = stdx::bitset<8, TestType>{};
    static_assert(not bs3.any());
}

TEMPLATE_TEST_CASE("none", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs1 = stdx::bitset<8, TestType>{};
    static_assert(bs1.none());

    constexpr auto bs2 = stdx::bitset<8, TestType>{8ul};
    static_assert(not bs2.none());
}

TEMPLATE_TEST_CASE("count", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs1 = stdx::bitset<8, TestType>{};
    static_assert(bs1.count() == 0u);

    constexpr auto bs2 = stdx::bitset<8, TestType>{0b10101ul};
    static_assert(bs2.count() == 3u);
}

TEMPLATE_TEST_CASE("or", "[bitset]", std::uint8_t, std::uint16_t, std::uint32_t,
                   std::uint64_t) {
    constexpr auto bs1 = stdx::bitset<3, TestType>{0b101ul};
    constexpr auto bs2 = stdx::bitset<3, TestType>{0b010ul};
    static_assert((bs1 | bs2) == stdx::bitset<3, TestType>{0b111ul});
}

TEMPLATE_TEST_CASE("and", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs1 = stdx::bitset<3, TestType>{0b101ul};
    constexpr auto bs2 = stdx::bitset<3, TestType>{0b100ul};
    static_assert((bs1 & bs2) == stdx::bitset<3, TestType>{0b100ul});
}

TEMPLATE_TEST_CASE("xor", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs1 = stdx::bitset<3, TestType>{0b101ul};
    constexpr auto bs2 = stdx::bitset<3, TestType>{0b010ul};
    static_assert((bs1 ^ bs2) == stdx::bitset<3, TestType>{0b111ul});
}

TEMPLATE_TEST_CASE("not", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<3, TestType>{0b101ul};
    static_assert(~bs == stdx::bitset<3, TestType>{0b10ul});
}

TEMPLATE_TEST_CASE("left shift", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<3, TestType>{0b101ul};
    static_assert(bs << 1u == stdx::bitset<3, TestType>{0b10ul});
    static_assert(bs << 2u == stdx::bitset<3, TestType>{0b100ul});
}

TEMPLATE_TEST_CASE("left shift (equal to type size)", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto type_size = std::numeric_limits<TestType>::digits;
    constexpr auto sz = type_size * 3;
    auto bs = stdx::bitset<sz, TestType>{};
    bs.set();
    bs <<= type_size;
    CHECK(bs[type_size]);
    CHECK(not bs[type_size - 1]);
    CHECK(not bs[0]);
}

TEMPLATE_TEST_CASE("left shift (off end)", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<3, TestType>{0b101ul};
    static_assert(bs << 3u == stdx::bitset<3, TestType>{});
}

TEMPLATE_TEST_CASE("left shift (large shift)", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto sz = std::numeric_limits<TestType>::digits * 2;
    constexpr auto shift_amount = std::numeric_limits<TestType>::digits + 1;
    constexpr auto bs = stdx::bitset<sz, TestType>{0b101ul} << shift_amount;
    static_assert(not bs[0]);
    static_assert(bs[shift_amount]);
    static_assert(not bs[shift_amount + 1]);
    static_assert(bs[shift_amount + 2]);
}

TEMPLATE_TEST_CASE("right shift", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<3, TestType>{0b101ul};
    static_assert(bs >> 1u == stdx::bitset<3, TestType>{0b10ul});
    static_assert(bs >> 2u == stdx::bitset<3, TestType>{0b1ul});
}

TEMPLATE_TEST_CASE("right shift (equal to type size)", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto type_size = std::numeric_limits<TestType>::digits;
    constexpr auto sz = type_size * 3;
    auto bs = stdx::bitset<sz, TestType>{};
    bs.set();
    bs >>= type_size;
    CHECK(not bs[type_size * 2]);
    CHECK(bs[type_size * 2 - 1]);
    CHECK(bs[0]);
}

TEMPLATE_TEST_CASE("right shift (off end)", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<3, TestType>{0b101ul};
    static_assert(bs >> 3u == stdx::bitset<3, TestType>{});
}

TEMPLATE_TEST_CASE("right shift (large shift)", "[bitset]", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto sz = std::numeric_limits<TestType>::digits * 2;
    constexpr auto shift_amount = std::numeric_limits<TestType>::digits + 1;
    auto bs = stdx::bitset<sz, TestType>{};
    bs.set();
    bs >>= shift_amount;
    CHECK(not bs[std::numeric_limits<TestType>::digits]);
    CHECK(not bs[std::numeric_limits<TestType>::digits - 1]);
    CHECK(bs[std::numeric_limits<TestType>::digits - 2]);
    CHECK(bs[0]);
}

TEMPLATE_TEST_CASE("for_each", "[bitset]", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<64, TestType>{0x01020304'05060708ul};
    auto result = decltype(bs){};
    for_each([&](auto i) { result.set(i); }, bs);
    CHECK(result == bs);
}

TEMPLATE_TEST_CASE("for_each iterates in order lsb to msb", "[bitset]",
                   std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t) {
    constexpr auto bs = stdx::bitset<5, TestType>{0b10101ul};
    auto result = decltype(bs){};
    std::size_t expected{};
    for_each(
        [&](auto i) {
            CHECK(i == expected);
            expected += 2;
            result.set(i);
        },
        bs);
    CHECK(result == bs);
}
