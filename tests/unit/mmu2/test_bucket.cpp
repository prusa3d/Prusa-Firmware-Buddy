#include <mmu2/fail_bucket.hpp>

#include "config_store/store_instance.hpp"

#include <catch2/catch.hpp>

TEST_CASE("MMU Fail leaky bucket") {
    config_store().mmu_fail_bucket.set(0);

    MMU2::FailLeakyBucket bucket(100, 5);

    REQUIRE_FALSE(bucket.reached_limit());

    SECTION("All successes") {
        for (uint32_t i = 0; i < 10000; i++) {
            bucket.success(i);

            REQUIRE_FALSE(bucket.reached_limit());
        }
    }

    SECTION("Burst") {
        for (uint32_t i = 0; i < 10; i++) {
            bucket.add_failure();
        }

        REQUIRE(bucket.reached_limit());
    }

    SECTION("Creep slowly") {
        uint32_t total_s = 0;
        for (uint32_t i = 0; i < 4; i++) {
            bucket.success(++total_s);

            bucket.add_failure();

            for (size_t j = 0; j < 2; j++) {
                bucket.success(++total_s);
            }
        }

        // Not yet, the successes slow the filling up a bit.
        REQUIRE_FALSE(bucket.reached_limit());

        for (uint32_t i = 0; i < 50; i++) {
            bucket.success(++total_s);

            bucket.add_failure();

            bucket.success(++total_s);
        }

        // But this takes it over.
        REQUIRE(bucket.reached_limit());
    }

    // Here we have _some_ failures, but not enough to ever overflow.
    SECTION("Fail a little") {
        uint32_t total_s = 0;
        for (uint32_t i = 0; i < 1000; i++) {
            bucket.add_failure();
            for (uint32_t j = 0; j < 100; j++) {
                bucket.success(++total_s);
                REQUIRE_FALSE(bucket.reached_limit());
            }
        }
    }
}
