#include "../internal/support.hpp"
#include <cthash/sha2/sha384.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

TEST_CASE("sha384 basics") {
    constexpr auto v1 = cthash::sha384 {}.update("").final();
    auto v1r = cthash::sha384 {}.update(runtime_pass("")).final();
    REQUIRE(v1 == "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"_sha384);
    REQUIRE(v1 == v1r);

    constexpr auto v2 = cthash::sha384 {}.update("hana").final();
    auto v2r = cthash::sha384 {}.update(runtime_pass("hana")).final();
    REQUIRE(v2 == "f365fd3e040e79a664d21de719128557b1188463d2d92be43522ccd4c316958b29f1189750b0f8d55aca50b8492982e8"_sha384);
    REQUIRE(v2 == v2r);

    constexpr auto v3 = cthash::sha384 {}.update(array_of_zeros<32>()).final();
    auto v3r = cthash::sha384 {}.update(runtime_pass(array_of_zeros<32>())).final();
    REQUIRE(v3 == "a38fff4ba26c15e4ac9cde8c03103ac89080fd47545fde9446c8f192729eab7bd03a4d5c3187f75fe2a71b0ee50a4a40"_sha384);
    REQUIRE(v3 == v3r);

    constexpr auto v4 = cthash::sha384 {}.update(array_of_zeros<64>()).final();
    auto v4r = cthash::sha384 {}.update(runtime_pass(array_of_zeros<64>())).final();
    REQUIRE(v4 == "c516aa8d3b457c636c6826937099c0d23a13f2c3701a388b3c8fe4bc2073281b0c4462610369884c4ababa8e97b6debe"_sha384);
    REQUIRE(v4 == v4r);

    constexpr auto v5 = cthash::sha384 {}.update(array_of_zeros<120>()).final();
    auto v5r = cthash::sha384 {}.update(runtime_pass(array_of_zeros<120>())).final();
    REQUIRE(v5 == "7212d895f4250ce1daa72e9e0caaef7132aed2e965885c55376818e45470de06fb6ebf7349c62fd342043f18010e46ac"_sha384);
    REQUIRE(v5 == v5r);

    constexpr auto v6 = cthash::sha384 {}.update(array_of_zeros<128>()).final();
    auto v6r = cthash::sha384 {}.update(runtime_pass(array_of_zeros<128>())).final();
    REQUIRE(v6 == "f809b88323411f24a6f152e5e9d9d1b5466b77e0f3c7550f8b242c31b6e7b99bcb45bdecb6124bc23283db3b9fc4f5b3"_sha384);
    REQUIRE(v6 == v6r);

    constexpr auto v7 = cthash::sha384 {}.update(array_of_zeros<512, char>()).final();
    auto v7r = cthash::sha384 {}.update(runtime_pass(array_of_zeros<512, char>())).final();
    REQUIRE(v7 == "d83d9a38c238ef3b7bc207bbea3287a8b37b37e731480a8d240d2a6953086c5ecbdf7ee4c72fec3a3e9d4a87f4f9b4fe"_sha384);
    REQUIRE(v7 == v7r);
}
