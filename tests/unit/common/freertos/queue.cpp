#include "catch2/catch.hpp"
#include "freertos_queue.hpp"

TEST_CASE("Freertos queue", "[freertos]") {
    SECTION("no dynamic allocation") {
        auto before = sbrk(0);
        {
            freertos::Queue<int, 10> queue;
            REQUIRE(before == sbrk(0));
            queue.send(10);
            REQUIRE(before == sbrk(0));
            int v;
            queue.receive(v);
            REQUIRE(before == sbrk(0));
        }
        REQUIRE(before == sbrk(0));
    }
    SECTION("send/receive") {
        freertos::Queue<int, 10> queue;
        for (int i = 0; i < 10; ++i) {
            REQUIRE(queue.send(i, 0));
        }
        REQUIRE(!queue.send(10, 0));
        int v;
        for (int i = 0; i < 10; ++i) {
            REQUIRE(queue.receive(v, 0));
            REQUIRE(v == i);
        }
        REQUIRE(!queue.receive(v, 0));
    }
}
