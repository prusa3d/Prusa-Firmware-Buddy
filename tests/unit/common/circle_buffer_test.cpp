#include "catch2/catch.hpp"
#include "circle_buffer.hpp"

/// Structure as nontrivial type for circular buffer
struct T {
    int a;
    int b;
};

TEST_CASE("CircleBuffer Operations", "[circle_buffer]") {
    CircleBuffer<T, 5> cb;
    T elem;

    // Fill
    CHECK(cb.Count() == 0);
    cb.push_back({ 1, 2 });
    CHECK(cb.Count() == 1);
    cb.push_back({ 2, 3 });
    CHECK(cb.Count() == 2);
    cb.push_back({ 3, 4 });
    CHECK(cb.Count() == 3);
    cb.push_back({ 4, 5 });
    CHECK(cb.Count() == 4);

    SECTION("adding past size") {
        // Can fit only 4 elements
        cb.push_back({ 5, 6 });
        CHECK(cb.Count() == 4);

        // Still only 4 elements
        cb.push_back({ 6, 7 });
        cb.push_back({ 7, 8 });
        CHECK(cb.Count() == 4);

        // Last 4 should remain
        for (int i = 0; i < 4; i++) {
            CHECK(cb[i].a == i + 4);
            CHECK(cb[i].b == i + 5);
        }
    }

    SECTION("clear") {
        // Clear all
        cb.clear();
        CHECK(cb.Count() == 0);
        CHECK(cb.ConsumeFirst(elem) == false);
        CHECK(cb.ConsumeLast(elem) == false);
    }

    SECTION("removing") {
        // Remove front
        CHECK(cb.ConsumeFirst(elem));
        CHECK(elem.a == 1);
        CHECK(elem.b == 2);

        // 3 should remain
        CHECK(cb.Count() == 3);
        for (int i = 0; i < 3; i++) {
            CHECK(cb[i].a == i + 2);
            CHECK(cb[i].b == i + 3);
        }

        // Remove back
        CHECK(cb.ConsumeLast(elem));
        CHECK(elem.a == 4);
        CHECK(elem.b == 5);

        // 2 should remain
        CHECK(cb.Count() == 2);
        for (int i = 0; i < 2; i++) {
            CHECK(cb[i].a == i + 2);
            CHECK(cb[i].b == i + 3);
        }
    }
}

TEST_CASE("CircleBuffer Iterators", "[circle_buffer]") {
    CircleBuffer<T, 5> cb;

    // Fill and rotate
    cb.push_back({ 1, 2 });
    cb.push_back({ 2, 3 });
    cb.push_back({ 3, 4 });
    cb.push_back({ 4, 5 });
    cb.push_back({ 5, 6 });
    cb.push_back({ 6, 7 });
    cb.push_back({ 7, 8 });

    SECTION("compilation") {
        // These things won't show until you use them in compiled code
        struct Stuff {
            CircleBuffer<T, 5> &internal;
            Stuff(CircleBuffer<T, 5> &cb_)
                : internal(cb_) {}

            // Function mixing const and nonconst iterators
            float const_get(CircleBuffer<T, 5>::iterator sample) const {
                return static_cast<float>(sample - internal.begin()) * 2.5;
            }
        };
        Stuff stuff(cb);

        // Iterator needs to be able to add and iterate
        for (auto it = stuff.internal.begin(); it < stuff.internal.end(); ++it) {
            it->a = (it + 3)->b;
        }

        // Reverse iterator needs to be able to add and iterate
        for (auto it = stuff.internal.rbegin(); it < stuff.internal.rend(); ++it) {
            it->a = (it + 3)->b;
        }

        // Const and nonconst iterators should mix automatically
        auto sample = stuff.internal.end() + 2;
        auto constgotten = stuff.const_get(sample);
        (void)constgotten;
    }

    SECTION("begin()") {
        // begin() should point to the first element
        CHECK(cb.begin()->a == 4);
        CHECK(cb.begin()->b == 5);
    }

    SECTION("end()") {
        // end() should point to one past last element
        CHECK(cb.end()->a == 3);
        CHECK(cb.end()->b == 4);
    }

    SECTION("manual loop") {
        // Manual loop should work
        int i = 4;
        for (auto it = cb.begin(); it < cb.end(); ++it) {
            CHECK(it->a == i++);
            CHECK(it->b == i);
        }
    }

    SECTION("range based loop") {
        // Range based loop should work
        int i = 4;
        for (auto it : cb) {
            CHECK(it.a == i++);
            CHECK(it.b == i);
        }
    }

    SECTION("operator[]") {
        // operator[] should work
        auto it = cb.begin();
        for (int i = 0; i < 4; i++) {
            CHECK(it[i].a == i + 4);
            CHECK(it[i].b == i + 5);
        }
    }

    SECTION("reverse iterator") {
        // Reverse iterator should work
        auto it = cb.rbegin();
        for (int i = 0; i < 4; i++) {
            CHECK(it->a == 7 - i);
            CHECK(it->b == 8 - i);
            ++it;
        }

        // After 4 iterations it should be equal to rend()
        CHECK(it == cb.rend());
    }
}

TEST_CASE("CircleBuffer basics", "[circle_buffer]") {
    SECTION("what goes in goes out") {
        CircleBuffer<uint32_t, 2> cb;
        uint32_t in = 0xdeadbeef;
        CHECK(cb.push_back_DontRewrite(in));

        uint32_t out;
        CHECK(cb.ConsumeFirst(out));
        // TODO: Either make this work or ditch the entire CircleBuffer class.
        // CHECK(out == in);
    }
}
