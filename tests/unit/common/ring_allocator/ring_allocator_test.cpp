#include <ring_allocator.hpp>

#include <catch2/catch.hpp>
#include <algorithm>
#include <random>
#include <cstring>

namespace {

std::random_device rd;
std::mt19937 g(rd());

void do_test(buddy::RingAllocator &allocator, std::vector<size_t> sizes, bool shuffle, size_t fits_at_least) {
    std::vector<std::pair<void *, size_t>> allocated;

    for (auto size : sizes) {
        void *ptr = allocator.allocate(size);
        allocator.sanity_check();
        if (fits_at_least == sizes.size()) {
            REQUIRE(ptr != nullptr);
        }

        if (ptr != nullptr) {
            uint8_t byte = size;

            // Just use the memory a little bit.
            memset(ptr, byte, size);

            allocated.push_back(std::make_pair(ptr, size));
        }
    }

    REQUIRE(allocated.size() >= fits_at_least);

    if (shuffle) {
        std::shuffle(allocated.begin(), allocated.end(), g);
    }

    for (auto [ptr, orig_size] : allocated) {
        uint8_t *ptr_b = reinterpret_cast<uint8_t *>(ptr);
        uint8_t b = orig_size;
        for (size_t s = 0; s < orig_size; s++) {
            REQUIRE(ptr_b[s] == b);
        }
        allocator.free(ptr);
        allocator.sanity_check();
    }
}

} // namespace

TEST_CASE("Allocate and free success") {
    buddy::RingAllocator allocator(1024);
    std::vector<size_t> sizes;

    SECTION("Small") {
        sizes.push_back(10);
        sizes.push_back(10);
        sizes.push_back(10);
    }

    SECTION("large") {
        sizes.push_back(100);
        sizes.push_back(100);
        sizes.push_back(100);
    }

    SECTION("huge") {
        sizes.push_back(1000);
    }

    SECTION("Different") {
        sizes.push_back(10);
        sizes.push_back(100);
        sizes.push_back(13);
        sizes.push_back(22);
    }

    for (size_t i = 0; i < 100; i++) {
        do_test(allocator, sizes, i > 50, sizes.size());
    }
}

TEST_CASE("Allocate doesn't fit") {
    // Note: in the real deployment, the Record has 12 bytes, but on 64bit
    // system where the test runs, it's 24.
    buddy::RingAllocator allocator(1048);

    std::vector<size_t> sizes;
    sizes.push_back(500); // Fits
    sizes.push_back(500); // Fits (note: the overhead should be 12 bytes per allocation
    sizes.push_back(500); // Doesn't fit.

    for (size_t i = 0; i < 100; i++) {
        do_test(allocator, sizes, i > 50, 2);
    }
}

TEST_CASE("Big big small") {
    buddy::RingAllocator allocator(1024);

    std::vector<size_t> sizes;
    sizes.push_back(600); // Fits
    sizes.push_back(600); // Doesn't fit
    sizes.push_back(200); // Small enough to still fit.

    for (size_t i = 0; i < 100; i++) {
        do_test(allocator, sizes, i > 50, 2);
    }
}
