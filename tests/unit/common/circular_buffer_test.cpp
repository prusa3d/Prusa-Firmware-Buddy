#include <catch2/catch.hpp>
#include <common/circular_buffer.hpp>

TEST_CASE("CircularBuffer basics", "[circular_buffer]") {

    SECTION("empty on construction") {
        CircularBuffer<uint32_t, 4> cb;
        CHECK(cb.size() == 0);
    }

    SECTION("instantiation with custom type") {
        struct T {
            int a;
            int b;
        };
        CircularBuffer<T, 4> cb;
        CHECK(cb.size() == 0);
    }

    SECTION("what goes in goes out") {
        CircularBuffer<uint32_t, 4> cb;
        CHECK(cb.size() == 0);

        uint32_t in = 0xdeadbeef;
        CHECK(cb.try_put(in));
        CHECK(cb.size() == 1);

        uint32_t out;
        CHECK(cb.try_get(out));
        CHECK(out == in);
        CHECK(cb.size() == 0);
    }

    SECTION("playing with the capacity") {
        CircularBuffer<uint32_t, 4> cb;
        CHECK(cb.size() == 0);

        // put 3 items
        CHECK(cb.try_put(0x11111111));
        CHECK(cb.size() == 1);
        CHECK(cb.try_put(0x22222222));
        CHECK(cb.size() == 2);
        CHECK(cb.try_put(0x33333333));
        CHECK(cb.size() == 3);

        // can't put 4th item
        CHECK(!cb.try_put(0x44444444));
        CHECK(cb.size() == 3);

        // get 1st item
        uint32_t dummy;
        CHECK(cb.try_get(dummy));
        CHECK(dummy == 0x11111111);
        CHECK(cb.size() == 2);

        // can put 4th item now
        CHECK(cb.try_put(0x44444444));
        CHECK(cb.size() == 3);

        // get all items
        CHECK(cb.try_get(dummy));
        CHECK(dummy == 0x22222222);
        CHECK(cb.size() == 2);
        CHECK(cb.try_get(dummy));
        CHECK(dummy == 0x33333333);
        CHECK(cb.size() == 1);
        CHECK(cb.try_get(dummy));
        CHECK(dummy == 0x44444444);
        CHECK(cb.size() == 0);

        // can't get more
        CHECK(!cb.try_get(dummy));
        CHECK(cb.size() == 0);

        // put one more
        CHECK(cb.try_put(0x55555555));
        CHECK(cb.size() == 1);

        // clear
        cb.clear();
        CHECK(cb.size() == 0);
        CHECK(!cb.try_get(dummy));
        CHECK(cb.try_put(dummy));
    }
}
