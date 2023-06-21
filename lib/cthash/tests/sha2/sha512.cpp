#include "../internal/support.hpp"
#include <cthash/sha2/sha512.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

template <typename Id>
struct identify;

TEST_CASE("sha512 zero staging should be empty") {
    const auto block = array_of_zeros<128>();
    const auto staging = cthash::internal_hasher<cthash::sha512_config>::build_staging(block);

    for (auto val : staging) {
        REQUIRE(val == static_cast<decltype(val)>(0));
    }
}

TEST_CASE("sha512 internal buffer at the end (two bytes)") {
    using sha512_hasher = cthash::internal_hasher<cthash::sha512_config>;

    auto h = sha512_hasher {};
    h.update_to_buffer_and_process<std::byte>(std::array<std::byte, 2> { std::byte { 'a' }, std::byte { 'b' } });
    h.finalize();

    // message
    REQUIRE(unsigned(h.block[0]) == unsigned('a'));
    REQUIRE(unsigned(h.block[1]) == unsigned('b'));

    // terminator
    REQUIRE(unsigned(h.block[2]) == 0b1000'0000u);

    // bit length
    REQUIRE(unsigned(h.block[127]) == 16u); // 2*8 = 16

    STATIC_REQUIRE(h.block.size() == 128u);

    // rest of the block must be zeros
    for (int i = 0; i != 128; ++i) {
        if (i > 2 && i < 127) {
            REQUIRE(unsigned(h.block[static_cast<size_t>(i)]) == unsigned { 0b0000'0000u });
        }
    }
}

TEST_CASE("sha512 internal buffer at the end (111B)") {
    using sha512_hasher = cthash::internal_hasher<cthash::sha512_config>;

    auto h = sha512_hasher {};
    h.update_to_buffer_and_process<std::byte>(array_of<111>(std::byte { 42 }));
    h.finalize();

    // message
    for (int i = 0; i != 111; ++i) {
        REQUIRE(unsigned(h.block[static_cast<size_t>(i)]) == unsigned(42));
    }

    // terminator
    REQUIRE(unsigned(h.block[111]) == 0b1000'0000u);

    // bit length
    REQUIRE(unsigned(h.block[112]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[113]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[114]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[115]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[116]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[117]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[118]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[119]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[120]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[121]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[122]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[123]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[124]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[125]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[126]) == 0b0000'0011u);
    REQUIRE(unsigned(h.block[127]) == 0b0111'1000u);

    STATIC_REQUIRE(h.block.size() == 128u);
}

TEST_CASE("sha512 internal buffer at the end (112B, first block)") {
    using sha512_hasher = cthash::internal_hasher<cthash::sha512_config>;

    auto h = sha512_hasher {};
    h.update_to_buffer_and_process<std::byte>(array_of<112>(std::byte { 42 }));
    const bool overflow = sha512_hasher::finalize_buffer(h.block, h.block_used);
    REQUIRE(overflow);

    // there is no message (as it was in previous block)
    for (int i = 0; i != 112; ++i) {
        REQUIRE(unsigned(h.block[static_cast<size_t>(i)]) == 42u);
    }

    // terminator
    REQUIRE(unsigned(h.block[112]) == 0b1000'0000u);

    // zero-padding
    for (int i = 113; i != 128; ++i) {
        REQUIRE(unsigned(h.block[static_cast<size_t>(i)]) == 0u);
    }

    STATIC_REQUIRE(h.block.size() == 128u);
}

TEST_CASE("sha512 internal buffer at the end (112B, second block)") {
    using sha512_hasher = cthash::internal_hasher<cthash::sha512_config>;

    auto h = sha512_hasher {};
    h.update_to_buffer_and_process<std::byte>(array_of<112>(std::byte { 42 }));
    h.finalize();

    // there is no message (as it was in previous block)
    for (int i = 0; i != 112; ++i) {
        REQUIRE(unsigned(h.block[static_cast<size_t>(i)]) == unsigned(0));
    }

    // bit length
    REQUIRE(unsigned(h.block[112]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[113]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[114]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[115]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[116]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[117]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[118]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[119]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[120]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[121]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[122]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[123]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[124]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[125]) == 0b0000'0000u);
    REQUIRE(unsigned(h.block[126]) == 0b0000'0011u);
    REQUIRE(unsigned(h.block[127]) == 0b1000'0000u);

    STATIC_REQUIRE(h.block.size() == 128u);
}

TEST_CASE("sha512 empty input") {
    const auto block = [] {
        auto r = array_of_zeros<128>();
        r[0] = std::byte { 0b1000'0000 };
        return r;
    }();
    const auto staging = cthash::internal_hasher<cthash::sha512_config>::build_staging(block);

    auto it = staging.begin();

    // from the block
    REQUIRE(*it++ == 0x80000000'00000000ull);
    for (int i = 1; i != 16; ++i) {
        REQUIRE(*it++ == 0ull);
    }

    // calculated by staging function
    REQUIRE(staging[16] == 0x80000000'00000000ull);
    REQUIRE(80 == staging.size());

    REQUIRE(staging[17] == 0x0000000000000000ull);
    REQUIRE(staging[18] == 0x0200100000000004ull);
    REQUIRE(staging[19] == 0x0000000000000000ull);
    REQUIRE(staging[20] == 0x1008000002000020ull);
    REQUIRE(staging[21] == 0x0000000000000000ull);
    REQUIRE(staging[22] == 0x8004220110080140ull);
    REQUIRE(staging[23] == 0x8000000000000000ull);
    REQUIRE(staging[24] == 0x0209108000400800ull);
    REQUIRE(staging[25] == 0x0400200000000008ull);
    REQUIRE(staging[26] == 0x1140a00320114028ull);
    REQUIRE(staging[27] == 0x3018000006000060ull);
    REQUIRE(staging[28] == 0xa24500b1180a2042ull);
    REQUIRE(staging[29] == 0x0010880440200500ull);
    REQUIRE(staging[30] == 0xd4a945c2a4270995ull);
    REQUIRE(staging[31] == 0x43ad128001402810ull);
    REQUIRE(staging[32] == 0xcb2a519703108414ull);
    REQUIRE(staging[33] == 0x2faad072c8658034ull);
    REQUIRE(staging[34] == 0x7d14cc9b14ba8338ull);
    REQUIRE(staging[35] == 0x9867c5d3eb13eeffull);
    REQUIRE(staging[36] == 0xc94dc04c92359678ull);
    REQUIRE(staging[37] == 0x92aba5ae1ee4675dull);
    REQUIRE(staging[38] == 0x00b560cd2d362129ull);
    REQUIRE(staging[39] == 0x682916bbbf1eb0d9ull);
    REQUIRE(staging[40] == 0xfd4ce54fb9c1fca2ull);
    REQUIRE(staging[41] == 0x389bbafedf01a2ffull);
    REQUIRE(staging[42] == 0xf80ad4187a704534ull);
    REQUIRE(staging[43] == 0xfc8a6db46b916fabull);
    REQUIRE(staging[44] == 0x0009308c4ca7d22bull);
    REQUIRE(staging[45] == 0x49437d8543e9c98cull);
    REQUIRE(staging[46] == 0x67f19b9756071662ull);
    REQUIRE(staging[47] == 0x23d1b6fd5980db71ull);
    REQUIRE(staging[48] == 0x044afa0ab38edb37ull);
    REQUIRE(staging[49] == 0x332fd4ee79f5c755ull);
    REQUIRE(staging[50] == 0xa55aa02bebfa6ad4ull);
    REQUIRE(staging[51] == 0xd7c6eb0c613c793aull);
    REQUIRE(staging[52] == 0x0d8e6b8077ea417cull);
    REQUIRE(staging[53] == 0xd647ca451bde7ccbull);
    REQUIRE(staging[54] == 0xb69eec4d0e03246full);
    REQUIRE(staging[55] == 0xc913f017b2ac853aull);
    REQUIRE(staging[56] == 0x66221af93a4225bbull);
    REQUIRE(staging[57] == 0x022066fa965f2dbfull);
    REQUIRE(staging[58] == 0x98519af83ac7c4d8ull);
    REQUIRE(staging[59] == 0xa9d9b257ea80f60full);
    REQUIRE(staging[60] == 0xb741f8b59c416c21ull);
    REQUIRE(staging[61] == 0xa3de4a86a2cdbefbull);
    REQUIRE(staging[62] == 0xa6e929fabf4b5fa6ull);
    REQUIRE(staging[63] == 0xeabfcd669f8d15a1ull);
}

TEST_CASE("sha512 basics", "") {
    constexpr auto v1 = cthash::sha512 {}.update("").final();
    auto v1r = cthash::sha512 {}.update(runtime_pass("")).final();
    REQUIRE(v1 == "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"_sha512);
    REQUIRE(v1 == v1r);

    constexpr auto v2 = cthash::sha512 {}.update("hana").final();
    auto v2r = cthash::sha512 {}.update(runtime_pass("hana")).final();
    REQUIRE(v2 == "74d15692038cd747dce0f4ff287ce1d5a7930c7e5948183419584f142039b3b25a94d1bf7f321f2fd2da37af1b6552f3e5bfc6c40d0bd8e16ecde338ee153a02"_sha512);
    REQUIRE(v2 == v2r);

    constexpr auto v3 = cthash::sha512 {}.update(array_of_zeros<96>()).final();
    auto v3r = cthash::sha512 {}.update(array_of_zeros<96>()).final();
    auto v3rb = cthash::sha512 {}.update(array_of_zeros<96, char>()).final();
    REQUIRE(v3 == "e866b15da9e5b18d4b3bde250fc08a208399440f37471313c5b4006e4151b0f4464b2cd7246899935d58660c0749cd11570bb8240760a6e46bb175be18cdaffe"_sha512);
    REQUIRE(v3 == v3r);
    REQUIRE(v3 == v3rb);

    constexpr auto v4 = cthash::sha512 {}.update(array_of_zeros<120>()).final();
    auto v4r = cthash::sha512 {}.update(array_of_zeros<120, char>()).final();
    REQUIRE(v4 == "c106c47ad6eb79cd2290681cb04cb183effbd0b49402151385b2d07be966e2d50bc9db78e00bf30bb567ccdd3a1c7847260c94173ba215a0feabb0edeb643ff0"_sha512);
    REQUIRE(v4 == v4r);

    constexpr auto v5 = cthash::sha512 {}.update(array_of_zeros<128>()).final();
    auto v5r = cthash::sha512 {}.update(array_of_zeros<128, char>()).final();
    REQUIRE(v5 == "ab942f526272e456ed68a979f50202905ca903a141ed98443567b11ef0bf25a552d639051a01be58558122c58e3de07d749ee59ded36acf0c55cd91924d6ba11"_sha512);
    REQUIRE(v5 == v5r);

    constexpr auto v6 = cthash::sha512 {}.update(array_of_zeros<512>()).final();
    auto v6r = cthash::sha512 {}.update(array_of_zeros<512, char>()).final();
    auto v6rb = cthash::sha512 {}.update(array_of_zeros<512>()).final();
    REQUIRE(v6 == "df40d4a774e0b453a5b87c00d6f0ef5d753143454e88ee5f7b607134598294c7905ccbcf94bbc46e474db6eb44e56a6dbb6d9a1be9d4fb5d1b5f2d0c6ed34bfe"_sha512);
    REQUIRE(v6 == v6r);
    REQUIRE(v6 == v6rb);
}
