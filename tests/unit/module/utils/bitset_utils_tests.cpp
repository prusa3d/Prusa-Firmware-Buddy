#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <utils/bitset_utils.hpp>

namespace {

/// Collects set bit indices via SetBitsIterator, in iteration order.
template <size_t size>
std::vector<size_t> collect(const std::bitset<size> &bitset) {
    std::vector<size_t> result;
    for (size_t bit : SetBitsIterator { bitset }) {
        result.push_back(bit);
    }
    return result;
}

} // namespace

TEST_CASE("SetBitsIterator") {
    SECTION("Empty bitset yields nothing") {
        REQUIRE(collect(std::bitset<8> {}).empty());
    }

    SECTION("Single bit") {
        REQUIRE(collect(std::bitset<8> { 0b00000001 }) == std::vector<size_t> { 0 });
        REQUIRE(collect(std::bitset<8> { 0b10000000 }) == std::vector<size_t> { 7 });
    }

    SECTION("Multiple bits are yielded from lowest to highest") {
        REQUIRE(collect(std::bitset<8> { 0b10010110 }) == std::vector<size_t> { 1, 2, 4, 7 });
    }

    SECTION("All bits set") {
        REQUIRE(collect(std::bitset<4> { 0b1111 }) == std::vector<size_t> { 0, 1, 2, 3 });
    }

    SECTION("begin equals end for empty bitset") {
        SetBitsIterator it { std::bitset<8> {} };
        REQUIRE(it.begin() == it.end());
    }

    SECTION("begin differs from end for non-empty bitset") {
        SetBitsIterator it { std::bitset<8> { 0b1 } };
        REQUIRE(it.begin() != it.end());
    }

    SECTION("Post-increment returns the previous position") {
        SetBitsIterator it { std::bitset<8> { 0b0110 } };
        auto prev = it++;
        REQUIRE(*prev == 1);
        REQUIRE(*it == 2);
    }
}

TEST_CASE("bitset_flood_fill") {
    using Bits = std::bitset<8>;

    SECTION("Empty input stays empty") {
        // f is never called, so even an all-adding f cannot grow the result.
        const auto result = bitset_flood_fill(Bits {}, [](size_t) { return ~Bits {}; });
        REQUIRE(result == Bits {});
    }

    SECTION("Result is reflexive - input is always kept") {
        const Bits input { 0b0101 };
        const auto result = bitset_flood_fill(input, [](size_t) { return Bits {}; });
        REQUIRE(result == input);
    }

    SECTION("Transitively adds dependencies") {
        // 0 -> 1 -> 2, nothing else.
        const auto f = [](size_t bit) -> Bits {
            switch (bit) {
            case 0:
                return Bits { 0b010 };
            case 1:
                return Bits { 0b100 };
            default:
                return Bits {};
            }
        };
        REQUIRE(bitset_flood_fill(Bits { 0b001 }, f) == Bits { 0b111 });
    }

    SECTION("Cycles terminate") {
        // 0 -> 1 -> 0
        const auto f = [](size_t bit) -> Bits {
            return bit == 0 ? Bits { 0b10 } : Bits { 0b01 };
        };
        REQUIRE(bitset_flood_fill(Bits { 0b01 }, f) == Bits { 0b11 });
    }

    SECTION("Self reference terminates") {
        const auto f = [](size_t) { return Bits { 0b1 }; };
        REQUIRE(bitset_flood_fill(Bits { 0b1 }, f) == Bits { 0b1 });
    }

    SECTION("f is called exactly once per resulting bit") {
        std::bitset<8> called;
        const auto f = [&](size_t bit) -> Bits {
            REQUIRE_FALSE(called.test(bit));
            called.set(bit);
            return bit == 0 ? Bits { 0b110 } : Bits {};
        };
        const auto result = bitset_flood_fill(Bits { 0b001 }, f);
        REQUIRE(result == Bits { 0b111 });
        REQUIRE(called == result);
    }
}
