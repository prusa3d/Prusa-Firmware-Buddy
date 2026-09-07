#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <concepts>

#include <core/vector.hpp>

namespace {
struct Tag {};
struct OtherTag {}; // a distinct strong-type tag, used by the "must not compile" checks below
template <size_t N>
using Vec = Vector<float, Tag, N>;
using Vec2 = Vec<2>;
using Vec3 = Vec<3>;
using Vec4 = Vec<4>;
} // namespace

// Layout: contiguous, no padding (this is what makes pos[] alias x/y/z/e)
static_assert(sizeof(Vec2) == 2 * sizeof(float));
static_assert(sizeof(Vec3) == 3 * sizeof(float));
static_assert(sizeof(Vec4) == 4 * sizeof(float));
static_assert(sizeof(Vector<bool, Tag, 4>) == 4 * sizeof(bool));
static_assert(Vec4::size() == 4);

// Vectors never convert implicitly - not across sizes, not across tags. (The size-up cast
// exists but is explicit; everything else is absent.)
static_assert(!std::convertible_to<Vec4, Vec2>);
static_assert(!std::convertible_to<Vec2, Vec4>);
static_assert(!std::convertible_to<Vec4, Vector<float, OtherTag, 4>>);

// Mixing tags in an operation is rejected - different-tag vectors aren't even comparable.
static_assert(!std::equality_comparable_with<Vec4, Vector<float, OtherTag, 4>>);

TEST_CASE("Vector layout & accessors", "[vector]") {
    CHECK(Vec3 {} == Vec3 { 0, 0, 0 }); // default construction zeroes everything

    Vec4 v { 1, 2, 3, 4 };
    CHECK(v.e == 4); // E is the last component...
    CHECK(v[3] == 4); // ...and pos[] aliases it E-last
    CHECK(v[0] == 1);

    CHECK(v.a == v.x); // alternative names
    CHECK(v.b == v.y);
    CHECK(v.c == v.z);
    CHECK(v._e == v.e);

    v[1] = 9; // writes through pos[] are visible via the named member
    CHECK(v.y == 9);
}

TEST_CASE("Vector arithmetic", "[vector]") {
    CHECK(Vec3 { 1, 2, 3 } + Vec3 { 10, 20, 30 } == Vec3 { 11, 22, 33 });
    CHECK(Vec3 { 1, 2, 3 } - Vec3 { 1, 1, 1 } == Vec3 { 0, 1, 2 });
    CHECK(Vec3 { 1, 2, 3 } * Vec3 { 2, 3, 4 } == Vec3 { 2, 6, 12 });
    CHECK(-Vec2 { 1, -2 } == Vec2 { -1, 2 });

    // Scalar operands apply to every component
    CHECK(Vec3 { 1, 2, 3 } + 10 == Vec3 { 11, 12, 13 });
    CHECK(Vec3 { 1, 2, 3 } - 1 == Vec3 { 0, 1, 2 });
    CHECK(Vec3 { 1, 2, 3 } * 2 == Vec3 { 2, 4, 6 });
    CHECK(Vec4 { 2, 4, 6, 8 } / 2 == Vec4 { 1, 2, 3, 4 });

    // Smaller right operand: trailing components of the left are left untouched
    CHECK(Vec3 { 1, 2, 3 } + Vec2 { 10, 20 } == Vec3 { 11, 22, 3 });
    CHECK(Vec3 { 1, 2, 3 } * Vec2 { 10, 20 } == Vec3 { 10, 40, 3 });
}

TEST_CASE("Vector compound assignment", "[vector]") {
    Vec4 v { 1, 2, 3, 4 };

    v += Vec4 { 1, 1, 1, 1 };
    CHECK(v == Vec4 { 2, 3, 4, 5 });

    v -= Vec2 { 1, 1 }; // smaller operand leaves the tail (z, e) untouched
    CHECK(v == Vec4 { 1, 2, 4, 5 });

    v *= Vec3 { 2, 2, 2 }; // e untouched
    CHECK(v == Vec4 { 2, 4, 8, 5 });

    v /= Vec2 { 2, 2 }; // z, e untouched
    CHECK(v == Vec4 { 1, 2, 8, 5 });

    // Scalars apply to every component
    v += 1;
    CHECK(v == Vec4 { 2, 3, 9, 6 });
    v -= 1;
    CHECK(v == Vec4 { 1, 2, 8, 5 });
    v *= 10;
    CHECK(v == Vec4 { 10, 20, 80, 50 });
    v /= 10;
    CHECK(v == Vec4 { 1, 2, 8, 5 });

    CHECK(&(v += Vec4 {}) == &v); // returns a reference to *this
}

TEST_CASE("Vector set", "[vector]") {
    Vec4 v { 1, 2, 3, 4 };

    v.set(5, 6); // only the first N components change
    CHECK(v == Vec4 { 5, 6, 3, 4 });

    v.set(Vec2 { 7, 8 }, 9); // vector prefix followed by scalars
    CHECK(v == Vec4 { 7, 8, 9, 4 });
}

TEST_CASE("Vector casts", "[vector]") {
    // Upcast fills the new components with 0
    CHECK(static_cast<Vec3>(Vec2 { 1, 2 }) == Vec3 { 1, 2, 0 });
    CHECK(static_cast<Vec4>(Vec2 { 1, 2 }) == Vec4 { 1, 2, 0, 0 });

    // Downcasts drop the trailing components
    CHECK(Vec4 { 1, 2, 3, 4 }.xy() == Vec2 { 1, 2 });
    CHECK(Vec4 { 1, 2, 3, 4 }.xyz() == Vec3 { 1, 2, 3 });
}

TEST_CASE("Vector magnitude", "[vector]") {
    CHECK(Vec2 { 3, 4 }.magnitude() == 5.f);
    CHECK(Vec3 { 2, 3, 6 }.magnitude() == 7.f);
}
