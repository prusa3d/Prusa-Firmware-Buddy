#include "catch2/catch_test_macros.hpp"
#include "circular_buffer.h"

TEST_CASE("circular_buffer::basic", "[circular_buffer]") {

    using CB = CircularBuffer<uint8_t, uint8_t, 32>;

    CB cb;

    // at the beginning the buffer is empty
    REQUIRE(cb.empty());

    // since its capacity was defined as 32, at least one element must be successfully inserted
    CHECK(cb.push(1));
    CHECK(cb.count() == 1);

    // is the element there?
    REQUIRE(!cb.empty());
    CHECK(cb.front() == 1);

    // remove the element
    uint8_t b = 0;
    CHECK(cb.pop(b));
    CHECK(b == 1);
    CHECK(cb.empty());
    CHECK(cb.count() == 0);
}

TEST_CASE("circular_buffer::fill", "[circular_buffer]") {

    static constexpr auto size = 4;
    using CB = CircularBuffer<uint8_t, uint8_t, size>;

    // start with an empty buffer
    CB cb;
    REQUIRE(cb.empty());
    REQUIRE(cb.count() == 0);

    // ensure we can fill the buffer
    for (auto i = 0; i != size; ++i) {
        CHECK(!cb.full());
        cb.push(i);
        CHECK(cb.count() == i + 1);
    }
    REQUIRE(cb.full());
    REQUIRE(cb.count() == size);

    // ensure another push fails
    REQUIRE(!cb.push(0));
    REQUIRE(cb.count() == size);

    // retrieve all elements
    for (auto i = 0; i != size; ++i) {
        uint8_t v;
        CHECK(cb.pop(v));
        CHECK(v == i);
        REQUIRE(cb.count() == size - i - 1);
    }
    REQUIRE(cb.empty());
    REQUIRE(cb.count() == 0);
}

TEST_CASE("circular_buffer::reset", "[circular_buffer]") {

    static constexpr auto size = 2;
    using CB = CircularBuffer<uint8_t, uint8_t, size>;

    // start with an empty buffer
    CB cb;
    REQUIRE(cb.empty());

    // push four elements
    REQUIRE(cb.push(1));
    REQUIRE(cb.push(2));

    // Check there are elements in the buffer
    REQUIRE(cb.full());
    REQUIRE(!cb.empty());

    // Reset the buffer
    cb.reset();

    // Buffer should be empty now
    REQUIRE(cb.empty());
    REQUIRE(cb.count() == 0);
}

TEST_CASE("circular_buffer::wrap_around", "[circular_buffer]") {

    static constexpr auto size = 4;
    using CB = CircularBuffer<uint8_t, uint8_t, size>;

    // start with an empty buffer
    CB cb;
    REQUIRE(cb.empty());

    // test inverse logic
    REQUIRE(!cb.full());

    // add two elements to shift the internal offset
    uint8_t v;
    cb.push(size + 1);
    cb.pop(v);
    cb.push(size + 1);
    cb.pop(v);
    REQUIRE(cb.empty());

    // loop to test the internal cursor wrap-around logic
    // the number of loops needs to be equal or greater than the index type
    for (auto loop = 0; loop != 256; ++loop) {
        INFO("loop " << loop);

        // ensure we can fill the buffer
        for (auto i = 0; i != size; ++i) {
            CHECK(!cb.full());
            cb.push(i);
            CHECK(!cb.empty());
            CHECK(cb.count() == i + 1);
        }
        REQUIRE(cb.full());
        REQUIRE(!cb.empty());
        REQUIRE(cb.count() == size);

        // retrieve all elements
        for (auto i = 0; i != size; ++i) {
            uint8_t v;
            CHECK(cb.pop(v));
            CHECK(v == i);
            CHECK(cb.count() == size - i - 1);
        }
        REQUIRE(cb.empty());
    }
}

TEST_CASE("circular_buffer::wrap_around_np2", "[circular_buffer]") {

    // same as above, but using a non-power-of-two size
    static constexpr auto size = 3;
    using CB = CircularBuffer<uint8_t, uint8_t, size>;

    // start with an empty buffer
    CB cb;
    REQUIRE(cb.empty());

    // test inverse logic
    REQUIRE(!cb.full());

    // add two elements to shift the internal offset
    uint8_t v;
    cb.push(size + 1);
    cb.pop(v);
    cb.push(size + 1);
    cb.pop(v);
    REQUIRE(cb.empty());

    // loop to test the internal cursor wrap-around logic
    // the number of loops needs to be equal or greater than the index type
    for (auto loop = 0; loop != 256; ++loop) {
        INFO("loop " << loop);

        // ensure we can fill the buffer
        for (auto i = 0; i != size; ++i) {
            CHECK(!cb.full());
            cb.push(i);
            CHECK(!cb.empty());
            CHECK(cb.count() == i + 1);
        }
        REQUIRE(cb.full());
        REQUIRE(!cb.empty());
        REQUIRE(cb.count() == size);

        // retrieve all elements
        for (auto i = 0; i != size; ++i) {
            uint8_t v;
            CHECK(cb.pop(v));
            CHECK(v == i);
            CHECK(cb.count() == size - i - 1);
        }
        REQUIRE(cb.empty());
    }
}

TEST_CASE("circular_buffer::minimal_size", "[circular_buffer]") {

    using CB = CircularBuffer<uint8_t, uint8_t, 1>;

    // test a buffer with a minimal size (1 element)
    CB cb;

    // initial state
    REQUIRE(cb.empty());
    REQUIRE(!cb.full());

    // push one element
    REQUIRE(cb.push(1));
    REQUIRE(cb.full());
    REQUIRE(!cb.empty());
    REQUIRE(!cb.push(2));

    // retrieve the element
    uint8_t v;
    REQUIRE(cb.pop(v));
    REQUIRE(v == 1);
    REQUIRE(cb.empty());
    REQUIRE(!cb.pop(v));
}
