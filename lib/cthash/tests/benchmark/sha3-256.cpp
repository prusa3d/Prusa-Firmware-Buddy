#include "../internal/support.hpp"
#include <cthash/sha3/sha3-256.hpp>
#include <memory>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

TEST_CASE("sha3-256 measurements", "[keccak-bench]") {
    std::array<std::byte, 1024> input {};

    for (int i = 0; i != (int)input.size(); ++i) {
        input[size_t(i)] = static_cast<std::byte>(i);
    }

    BENCHMARK("16 byte input") {
        return cthash::sha3_256 {}.update(std::span(runtime_pass(input)).first(16)).final();
    };

    BENCHMARK("32 byte input") {
        return cthash::sha3_256 {}.update(std::span(runtime_pass(input)).first(32)).final();
    };

    BENCHMARK("48 byte input") {
        return cthash::sha3_256 {}.update(std::span(runtime_pass(input)).first(48)).final();
    };

    BENCHMARK("64 byte input") {
        return cthash::sha3_256 {}.update(std::span(runtime_pass(input)).first(64)).final();
    };

    BENCHMARK("96 byte input") {
        return cthash::sha3_256 {}.update(std::span(runtime_pass(input)).first(96)).final();
    };

    BENCHMARK("10kB input") {
        auto h = cthash::sha3_256 {};
        for (int i = 0; i != 10; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };

    BENCHMARK("1MB input") {
        auto h = cthash::sha3_256 {};
        for (int i = 0; i != 1024; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };

    BENCHMARK("2MB input") {
        auto h = cthash::sha3_256 {};
        for (int i = 0; i != 2 * 1024; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };

    BENCHMARK("4MB input") {
        auto h = cthash::sha3_256 {};
        for (int i = 0; i != 4 * 1024; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };

    BENCHMARK("8MB input") {
        auto h = cthash::sha3_256 {};
        for (int i = 0; i != 8 * 1024; ++i) {
            h.update(std::span(runtime_pass(input)).first(1024));
        }
        return h.final();
    };
}

#ifdef OPENSSL_BENCHMARK

    #include <openssl/evp.h>
    #include <openssl/sha.h>

struct EVP_destroy {
    void operator()(EVP_MD_CTX *ptr) noexcept {
        EVP_MD_CTX_destroy(ptr);
    }
};

using unique_evp_ptr = std::unique_ptr<EVP_MD_CTX, EVP_destroy>;

unique_evp_ptr openssl_sha3_256_init() {
    const EVP_MD *algorithm = EVP_sha3_256();
    REQUIRE(algorithm != nullptr);
    EVP_MD_CTX *context = EVP_MD_CTX_create();
    EVP_DigestInit_ex(context, algorithm, nullptr);
    return unique_evp_ptr(context);
}

template <cthash::byte_like Byte>
void openssl_sha3_256_update(unique_evp_ptr &ctx, std::span<const Byte> in) {
    EVP_DigestUpdate(ctx.get(), in.data(), in.size());
}

auto openssl_sha3_256_final(unique_evp_ptr &ctx) -> cthash::sha3_256_value {
    cthash::sha3_256_value out;
    unsigned length { 0 };
    EVP_DigestFinal_ex(ctx.get(), reinterpret_cast<unsigned char *>(out.data()), &length);
    REQUIRE(size_t(length) == out.size());
    return out;
}

TEST_CASE("check openssl first", "[openssl-test]") {
    using namespace std::string_view_literals;

    auto ctx = openssl_sha3_256_init();
    openssl_sha3_256_update(ctx, std::span<const char>("hanicka"sv));
    const auto res = openssl_sha3_256_final(ctx);
    bool correct = res == "8f8b0b8af4c371e91791b1ddb2d0788661dd687060404af6320971bcc53b44fb"_sha3_256;
    REQUIRE(correct);
}

TEST_CASE("openssl sha3-256 measurements", "[keccak-bench][openssl]") {
    std::array<std::byte, 1024> input {};

    for (int i = 0; i != (int)input.size(); ++i) {
        input[static_cast<size_t>(i)] = static_cast<std::byte>(i);
    }

    BENCHMARK("16 byte input") {
        auto ctx = openssl_sha3_256_init();
        const auto data = std::span(runtime_pass(input)).first(16);
        openssl_sha3_256_update(ctx, data);
        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("32 byte input") {
        auto ctx = openssl_sha3_256_init();
        const auto data = std::span(runtime_pass(input)).first(32);
        openssl_sha3_256_update(ctx, data);
        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("48 byte input") {
        auto ctx = openssl_sha3_256_init();
        const auto data = std::span(runtime_pass(input)).first(48);
        openssl_sha3_256_update(ctx, data);
        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("64 byte input") {
        auto ctx = openssl_sha3_256_init();
        const auto data = std::span(runtime_pass(input)).first(64);
        openssl_sha3_256_update(ctx, data);
        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("96 byte input") {
        auto ctx = openssl_sha3_256_init();
        const auto data = std::span(runtime_pass(input)).first(96);
        openssl_sha3_256_update(ctx, data);
        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("10kB input") {
        auto ctx = openssl_sha3_256_init();

        for (int i = 0; i != 10; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            openssl_sha3_256_update(ctx, data);
        }

        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("1MB input") {
        auto ctx = openssl_sha3_256_init();

        for (int i = 0; i != 1024; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            openssl_sha3_256_update(ctx, data);
        }

        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("2MB input") {
        auto ctx = openssl_sha3_256_init();

        for (int i = 0; i != 2 * 1024; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            openssl_sha3_256_update(ctx, data);
        }

        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("4MB input") {
        auto ctx = openssl_sha3_256_init();

        for (int i = 0; i != 4 * 1024; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            openssl_sha3_256_update(ctx, data);
        }

        return openssl_sha3_256_final(ctx);
    };

    BENCHMARK("8MB input") {
        auto ctx = openssl_sha3_256_init();

        for (int i = 0; i != 8 * 1024; ++i) {
            const auto data = std::span(runtime_pass(input)).first(1024);
            openssl_sha3_256_update(ctx, data);
        }

        return openssl_sha3_256_final(ctx);
    };
}

#endif
