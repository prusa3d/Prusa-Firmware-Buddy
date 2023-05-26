#include "../internal/support.hpp"
#include <cthash/sha2/sha512.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

TEST_CASE("sha512 measurements") {
    std::array<std::byte, 1024> input {};

    for (int i = 0; i != (int)input.size(); ++i) {
        input[static_cast<size_t>(i)] = static_cast<std::byte>(i);
    }

    BENCHMARK("16 byte input") {
        return cthash::sha512 {}.update(std::span(runtime_pass(input)).first(16)).final();
    };

    BENCHMARK("32 byte input") {
        return cthash::sha512 {}.update(std::span(runtime_pass(input)).first(32)).final();
    };

    BENCHMARK("48 byte input") {
        return cthash::sha512 {}.update(std::span(runtime_pass(input)).first(48)).final();
    };

    BENCHMARK("64 byte input") {
        return cthash::sha512 {}.update(std::span(runtime_pass(input)).first(64)).final();
    };

    BENCHMARK("96 byte input") {
        return cthash::sha512 {}.update(std::span(runtime_pass(input)).first(96)).final();
    };

    BENCHMARK("10kB input") {
        auto h = cthash::sha512 {};
        for (int i = 0; i != 10; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };

    BENCHMARK("1MB input") {
        auto h = cthash::sha512 {};
        for (int i = 0; i != 1024; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };

    BENCHMARK("10MB input") {
        auto h = cthash::sha512 {};
        for (int i = 0; i != 10 * 1024; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };
}

#ifdef OPENSSL_BENCHMARK

    #include <openssl/sha.h>

TEST_CASE("openssl sha512 measurements") {
    std::array<std::byte, 1024> input {};

    for (int i = 0; i != (int)input.size(); ++i) {
        input[static_cast<size_t>(i)] = static_cast<std::byte>(i);
    }

    BENCHMARK("16 byte input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);
        const auto data = std::span(runtime_pass(input)).first(16);
        SHA512_Update(&ctx, data.data(), data.size());
        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };

    BENCHMARK("32 byte input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);
        const auto data = std::span(runtime_pass(input)).first(32);
        SHA512_Update(&ctx, data.data(), data.size());
        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };

    BENCHMARK("48 byte input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);
        const auto data = std::span(runtime_pass(input)).first(48);
        SHA512_Update(&ctx, data.data(), data.size());
        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };

    BENCHMARK("64 byte input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);
        const auto data = std::span(runtime_pass(input)).first(64);
        SHA512_Update(&ctx, data.data(), data.size());
        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };

    BENCHMARK("96 byte input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);
        const auto data = std::span(runtime_pass(input)).first(96);
        SHA512_Update(&ctx, data.data(), data.size());
        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };

    BENCHMARK("10kB input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);

        for (int i = 0; i != 10; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            SHA512_Update(&ctx, data.data(), data.size());
        }

        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };

    BENCHMARK("1MB input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);

        for (int i = 0; i != 1024; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            SHA512_Update(&ctx, data.data(), data.size());
        }

        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };

    BENCHMARK("10MB input") {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);

        for (int i = 0; i != 10 * 1024; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            SHA512_Update(&ctx, data.data(), data.size());
        }

        std::array<unsigned char, 64> res;
        SHA512_Final(res.data(), &ctx);
        return res;
    };
}

#endif
