#include "../internal/support.hpp"
#include <cthash/sha3/sha3-256.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <iostream>

using namespace cthash::literals;

TEMPLATE_TEST_CASE("sha3 common xor_overwrite_block", "[xor]", std::byte, char) {
    using T = TestType;

    auto h = cthash::sha3_256();

    h.xor_overwrite_block(std::span<const T>(std::array<T, 4> { static_cast<T>(0xFAu), static_cast<T>(0xFBu), static_cast<T>(0xFCu), static_cast<T>(0xFDu) }));

    REQUIRE(h.position == 4u);

    REQUIRE(h.internal_state[0] == 0X00000000'FDFCFBFAull);
    REQUIRE(h.internal_state[1] == 0ull);

    h.xor_overwrite_block(std::span<const T>(std::array<T, 6> { static_cast<T>(0xF0u), static_cast<T>(0xF1u), static_cast<T>(0xF2u), static_cast<T>(0xF3u), static_cast<T>(0xA0u), static_cast<T>(0xA1u) }));

    REQUIRE(h.internal_state[0] == 0XF3F2F1F0'FDFCFBFAull);
    REQUIRE(h.internal_state[1] == 0X00000000'0000A1A0ull);
    REQUIRE(h.internal_state[2] == 0ull);

    REQUIRE(h.position == 10u);

    h.xor_overwrite_block(std::span<const T>(std::array<T, 33> {
        static_cast<T>(0xCCu),
        static_cast<T>(0xCCu),
        static_cast<T>(0xCCu),
        static_cast<T>(0xCCu),
        static_cast<T>(0xCCu),
        static_cast<T>(0xCCu),
        // aligned blocks
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        static_cast<T>(0xDDu),
        // suffix
        static_cast<T>(0xBBu),
        static_cast<T>(0xBBu),
        static_cast<T>(0xBBu) }));

    REQUIRE(h.internal_state[0] == 0XF3F2F1F0'FDFCFBFAull);
    REQUIRE(h.internal_state[1] == 0XCCCCCCCC'CCCCA1A0ull);
    REQUIRE(h.internal_state[2] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[3] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[4] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[5] == 0X00000000'00BBBBBBull);
    REQUIRE(h.internal_state[6] == 0ull);

    REQUIRE(h.position == 43u);

    h.xor_overwrite_block(std::span<const T>(std::array<T, 5> { static_cast<T>(0x11u), static_cast<T>(0x11u), static_cast<T>(0x11u), static_cast<T>(0x11u), static_cast<T>(0x11u) }));

    REQUIRE(h.internal_state[0] == 0XF3F2F1F0'FDFCFBFAull);
    REQUIRE(h.internal_state[1] == 0XCCCCCCCC'CCCCA1A0ull);
    REQUIRE(h.internal_state[2] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[3] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[4] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[5] == 0X11111111'11BBBBBBull);
    REQUIRE(h.internal_state[6] == 0ull);

    REQUIRE(h.position == 48u);

    h.xor_overwrite_block(std::span<const T>(std::array<T, 10> { static_cast<T>(0x23u), static_cast<T>(0x22u), static_cast<T>(0x22u), static_cast<T>(0x22u), static_cast<T>(0x22u), static_cast<T>(0x22u), static_cast<T>(0x22u), static_cast<T>(0x22u), static_cast<T>(0x44u), static_cast<T>(0x45u) }));

    REQUIRE(h.internal_state[0] == 0XF3F2F1F0'FDFCFBFAull);
    REQUIRE(h.internal_state[1] == 0XCCCCCCCC'CCCCA1A0ull);
    REQUIRE(h.internal_state[2] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[3] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[4] == 0XDDDDDDDD'DDDDDDDDull);
    REQUIRE(h.internal_state[5] == 0X11111111'11BBBBBBull);
    REQUIRE(h.internal_state[6] == 0X22222222'22222223ull);
    REQUIRE(h.internal_state[7] == 0X00000000'00004544ull);

    REQUIRE(h.position == 58u);
}

/*
TEST_CASE("sha3 common xor_overwrite_block final (old)", "[xor]") {
	auto h = cthash::sha3_256();

	REQUIRE(h.rate == (1088u / 8u)); // bytes

	h.final_absorb_old();
	REQUIRE(cthash::cast_from_le_bytes<uint64_t>(std::span<const std::byte>(h.buffer.storage).first<8u>()) == 0b0000'0110ull); // it's reverted
	REQUIRE(unsigned(h.buffer.storage[0]) == unsigned(std::byte{0b0000'0110u}));

	// in between
	for (unsigned i = 1; i != 16; ++i) {
		REQUIRE(cthash::cast_from_le_bytes<uint64_t>(std::span<const std::byte>(h.buffer.storage).subspan(i * 8u).template first<8u>()) == 0ull);
	}

	// end of padding
	constexpr auto end_of_padding = std::byte{0b1000'0000u};
	constexpr uint64_t end_of_padding_num = 0b1000'0000ull << (7u * 8u);
	REQUIRE(cthash::cast_from_le_bytes<uint64_t>(std::span<const std::byte>(h.buffer.storage).subspan(16u * 8u).first<8u>()) == end_of_padding_num);
	REQUIRE(unsigned(h.buffer.storage[135]) == unsigned(end_of_padding));
	REQUIRE(136 == h.buffer.storage.size());
	REQUIRE(h.rate == h.buffer.storage.size());
}*/

TEST_CASE("sha3 common xor_padding_block final", "[xor]") {
    auto h = cthash::sha3_256();
    REQUIRE(h.rate == (1088u / 8u)); // bytes
    h.xor_padding_block();
    REQUIRE(h.internal_state[0] == 0b0000'0110ull); // it's reverted

    // in between is all zero
    REQUIRE(h.internal_state[1] == 0ull);
    REQUIRE(h.internal_state[2] == 0ull);
    REQUIRE(h.internal_state[3] == 0ull);
    REQUIRE(h.internal_state[4] == 0ull);
    REQUIRE(h.internal_state[5] == 0ull);
    REQUIRE(h.internal_state[6] == 0ull);
    REQUIRE(h.internal_state[7] == 0ull);
    REQUIRE(h.internal_state[8] == 0ull);
    REQUIRE(h.internal_state[9] == 0ull);
    REQUIRE(h.internal_state[10] == 0ull);
    REQUIRE(h.internal_state[11] == 0ull);
    REQUIRE(h.internal_state[12] == 0ull);
    REQUIRE(h.internal_state[13] == 0ull);
    REQUIRE(h.internal_state[14] == 0ull);
    REQUIRE(h.internal_state[15] == 0ull);

    // end of padding
    constexpr uint64_t end_of_padding = 0b1000'0000ull << (7u * 8u);
    REQUIRE(h.internal_state[16] == end_of_padding);

    // capacity
    REQUIRE(h.internal_state[17] == 0ull);
    REQUIRE(h.internal_state[18] == 0ull);
    REQUIRE(h.internal_state[19] == 0ull);
    REQUIRE(h.internal_state[20] == 0ull);
    REQUIRE(h.internal_state[21] == 0ull);
    REQUIRE(h.internal_state[22] == 0ull);
    REQUIRE(h.internal_state[23] == 0ull);
    REQUIRE(h.internal_state[24] == 0ull);
}

constexpr auto to_span = []<typename T, size_t N>(const std::array<T, N> &in) {
    return std::span<const T>(in);
};

TEMPLATE_TEST_CASE("sha3 unaligned prefix", "[xor]", std::byte, char) {
    using T = TestType;

    SECTION("one") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u) }), 7u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0x12u } };
        REQUIRE(m0);
    }

    SECTION("two") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u), static_cast<T>(0x34u) }), 6u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0x12u }, std::byte { 0x34u } };
        REQUIRE(m0);
    }

    SECTION("three") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u), static_cast<T>(0x34u), static_cast<T>(0x56u) }), 5u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0x12u }, std::byte { 0x34u }, std::byte { 0x56u } };
        REQUIRE(m0);
    }

    SECTION("four") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u), static_cast<T>(0x34u), static_cast<T>(0x56u), static_cast<T>(0x78u) }), 4u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0x12u }, std::byte { 0x34u }, std::byte { 0x56u }, std::byte { 0x78u } };
        REQUIRE(m0);
    }

    SECTION("five") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u), static_cast<T>(0x34u), static_cast<T>(0x56u), static_cast<T>(0x78u), static_cast<T>(0x90u) }), 3u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0 }, std::byte { 0 }, std::byte { 0 }, std::byte { 0x12u }, std::byte { 0x34u }, std::byte { 0x56u }, std::byte { 0x78u }, std::byte { 0x90u } };
        REQUIRE(m0);
    }

    SECTION("six") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u), static_cast<T>(0x34u), static_cast<T>(0x56u), static_cast<T>(0x78u), static_cast<T>(0x90u), static_cast<T>(0xABu) }), 2u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0 }, std::byte { 0 }, std::byte { 0x12u }, std::byte { 0x34u }, std::byte { 0x56u }, std::byte { 0x78u }, std::byte { 0x90u }, std::byte { 0xABu } };
        REQUIRE(m0);
    }

    SECTION("seven") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u), static_cast<T>(0x34u), static_cast<T>(0x56u), static_cast<T>(0x78u), static_cast<T>(0x90u), static_cast<T>(0xABu), static_cast<T>(0xCDu) }), 1u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0 }, std::byte { 0x12u }, std::byte { 0x34u }, std::byte { 0x56u }, std::byte { 0x78u }, std::byte { 0x90u }, std::byte { 0xABu }, std::byte { 0xCDu } };
        REQUIRE(m0);
    }

    SECTION("eight") {
        const auto r0 = cthash::convert_prefix_into_aligned<uint64_t>(to_span(std::array { static_cast<T>(0x12u), static_cast<T>(0x34u), static_cast<T>(0x56u), static_cast<T>(0x78u), static_cast<T>(0x90u), static_cast<T>(0xABu), static_cast<T>(0xCDu), static_cast<T>(0xEFu) }), 0u);
        bool m0 = r0 == std::array<std::byte, 8> { std::byte { 0x12u }, std::byte { 0x34u }, std::byte { 0x56u }, std::byte { 0x78u }, std::byte { 0x90u }, std::byte { 0xABu }, std::byte { 0xCDu }, std::byte { 0xEFu } };
        REQUIRE(m0);
    }
}

TEMPLATE_TEST_CASE("sha3 sequence of overwrites", "[xor]", std::byte, char) {
    using T = TestType;

    auto h = cthash::sha3_256();

    auto in = std::array<TestType, 1> { static_cast<T>('*') };

    h.update(in);
    REQUIRE(h.position == 1u);
    REQUIRE(h.internal_state[0] == 0x2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 2u);
    REQUIRE(h.internal_state[0] == 0x2A2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 3u);
    REQUIRE(h.internal_state[0] == 0x2A2A2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 4u);
    REQUIRE(h.internal_state[0] == 0x2A2A2A2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 5u);
    REQUIRE(h.internal_state[0] == 0x2A2A2A2A2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 6u);
    REQUIRE(h.internal_state[0] == 0x2A2A2A2A2A2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 7u);
    REQUIRE(h.internal_state[0] == 0x2A2A2A2A2A2A2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 8u);
    REQUIRE(h.internal_state[0] == 0x2A2A2A2A2A2A2A2Aull);
    REQUIRE(h.internal_state[1] == 0x0ull);

    h.update(in);
    REQUIRE(h.position == 9u);
    REQUIRE(h.internal_state[0] == 0x2A2A2A2A2A2A2A2Aull);
    REQUIRE(h.internal_state[1] == 0x2Aull);
    REQUIRE(h.internal_state[2] == 0ull);
}
