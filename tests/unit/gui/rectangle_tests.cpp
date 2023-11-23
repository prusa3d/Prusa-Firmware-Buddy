#include "catch2/catch.hpp"

#include "guiapi/include/Rect16.h"

#include <vector>
#include <tuple>

/// Warning!
/// With yet unknown reason for us the method Catch::getResultCapture() returns nullptr in case the
/// benchmarks in CATCH2 are configured as enable. Please consider this issue when you'll decide to
/// write benchmark tests.
#define COMPARE_ARRAYS(lhs, rhs)                   compareArrays(Catch::getResultCapture().getCurrentTestName(), __LINE__, lhs, rhs)
#define COMPARE_ARRAYS_SIZE_FROM_SMALLER(lhs, rhs) compareArraysGetSizeFromSmaller(Catch::getResultCapture().getCurrentTestName(), __LINE__, lhs, rhs)

template <typename T>
void compareVectors(const std::string &test, unsigned line, const std::vector<T> &lhs, const std::vector<T> &rhs) {
    INFO("Test case [" << test << "] failed at line " << line); // Reported only if REQUIRE fails
    CHECK(lhs == rhs);
}

template <typename T, size_t N>
void compareArrays(const std::string &test, unsigned line, const std::array<T, N> &lhs, const std::array<T, N> &rhs) {
    std::vector<T> lv(lhs.begin(), lhs.end());
    std::vector<T> rv(rhs.begin(), rhs.end());
    compareVectors(test, line, lv, rv);
}

template <typename T, size_t N>
void compareArrays(const std::string &test, unsigned line, const T *lhs, const std::array<T, N> &rhs) {
    std::vector<T> lv(lhs, lhs + N);
    std::vector<T> rv(rhs.begin(), rhs.end());
    compareVectors(test, line, lv, rv);
}

template <typename T, size_t N>
void compareArrays(const std::string &test, unsigned line, const std::vector<T> &lhs, const std::array<T, N> &rhs) {
    std::vector<T> rv(rhs.begin(), rhs.end());
    compareVectors(test, line, lhs, rv);
}

template <typename T>
void compareArraysGetSizeFromSmaller(const std::string &test, unsigned line, const std::vector<T> &lhs, const std::vector<T> &rhs) {
    size_t min_len = std::min(rhs.size(), lhs.size());
    std::vector<T> lv(lhs.begin(), lhs.begin() + min_len);
    std::vector<T> rv(rhs.begin(), rhs.begin() + min_len);
    compareVectors(test, line, lv, rv);
}

template <typename T, size_t N>
void compareArraysGetSizeFromSmaller(const std::string &test, unsigned line, const std::vector<T> &lhs, const std::array<T, N> &rhs) {
    std::vector<T> rv(rhs.begin(), rhs.begin() + std::min(lhs.size(), N));
    compareVectors(test, line, lhs, rv);
}

template <typename T, size_t N>
void compareArraysGetSizeFromSmaller(const std::string &test, unsigned line, const std::array<T, N> &lhs, const std::vector<T> &rhs) {
    compareArraysGetSizeFromSmaller(test, line, rhs, lhs);
}

TEST_CASE("rectangle construc", "[rectangle]") {

    SECTION("topleft corner & width & height") {
        point_i16_t top_left = { 10, 20 };
        Rect16 r { top_left, 20, 40 };
        CHECK(r.Width() == 20);
        CHECK(r.Height() == 40);
    }

    SECTION("topleft corner & size") {
        point_i16_t top_left = { 10, 20 };
        size_ui16_t size = { 20, 40 };
        Rect16 r { top_left, size };
        CHECK(r.EndPoint().x == 30);
        CHECK(r.EndPoint().y == 60);
    }

    SECTION("empty box") {
        Rect16 r;
        CHECK(r.Width() == 0);
        CHECK(r.Height() == 0);
        CHECK(r.BeginPoint().x == 0);
        CHECK(r.BeginPoint().y == 0);
        CHECK(r.EndPoint().x == 0);
        CHECK(r.EndPoint().y == 0);
    }

    SECTION("by coordinates") {
        Rect16 r { 10, 20, 10, 10 };
        CHECK(r.BeginPoint().x == 10);
        CHECK(r.BeginPoint().y == 20);
        CHECK(r.Width() == 10);
        CHECK(r.Height() == 10);
    }

    SECTION("copy construct") {
        Rect16 q { 10, 20, 20, 40 };
        Rect16 r { q };
        CHECK(r.BeginPoint().x == 10);
        CHECK(r.BeginPoint().y == 20);
        CHECK(r.Width() == 20);
        CHECK(r.Height() == 40);
    }

    SECTION("2 points") {
        point_i16_t p0 = { 10, 20 };
        point_i16_t p1 = { 32, 48 };
        point_i16_t p2 = { 2, 48 };

        Rect16 r0 { p0, p1 };
        CHECK(r0.TopLeft().x == 10);
        CHECK(r0.TopLeft().y == 20);
        CHECK(r0.BottomRight().x == 32);
        CHECK(r0.BottomRight().y == 48);

        // same 2 points in different order must create same rect
        Rect16 r1 { p1, p0 };
        CHECK(r0 == r1);

        // p0 top-right
        // p2 bottom-left
        Rect16 r2 { p0, p2 };
        CHECK(r2.Top() == p0.y);
        CHECK(r2.Left() == p2.x);
        CHECK(r2.BottomRight().x == p0.x);
        CHECK(r2.BottomRight().y == p2.y);

        // same 2 points in different order must create same rect
        Rect16 r3 { p2, p0 };
        CHECK(r2 == r3);
    }

    SECTION("Top Left Bottom and Right accessors") {

        point_i16_t top_left, bot_right;

        std::tie(top_left, bot_right) = GENERATE(
            std::make_tuple<point_i16_t, point_i16_t>({ 0, 0 }, { 10, 20 }),
            std::make_tuple<point_i16_t, point_i16_t>({ 2, 4 }, { 10, 20 }),
            std::make_tuple<point_i16_t, point_i16_t>({ -10, -20 }, { 10, 20 }),
            std::make_tuple<point_i16_t, point_i16_t>({ -100, -200 }, { 10, 20 }));

        Rect16 r { top_left, bot_right };
        CHECK(r.Top() == top_left.y);
        CHECK(r.Left() == top_left.x);
        CHECK(r.Bottom() == bot_right.y);
        CHECK(r.Right() == bot_right.x);
    }

    // SECTION("by coordinates & wrong x") {
    //     Rect16 r { 10, 20, 0, 40 };
    //     CHECK(r.Width() == 0);
    //     CHECK(r.Height() != 20);
    // }

    // SECTION("by coordinates & wrong y") {
    //     Rect16 r { 10, 20, 20, 10 };
    //     CHECK(r.Width() != 10);
    //     CHECK(r.Height() != 0);
    // }

    SECTION("copy & shift") {
        Rect16 r, expected;
        ShiftDir_t dir;
        uint16_t offset;

        std::tie(r, dir, offset, expected) = GENERATE(
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Left, 20, { -10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Left, 10, { 0, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Left, 0, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Right, 20, { 30, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Right, 10, { 20, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Right, 0, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Top, 20, { 10, -10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Top, 10, { 10, 0, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Top, 0, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Bottom, 20, { 10, 30, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Bottom, 10, { 10, 20, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Bottom, 0, { 10, 10, 30, 30 }));

        Rect16 res { r, dir, offset };

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.BeginPoint().x == expected.BeginPoint().x);
        CHECK(res.BeginPoint().y == expected.BeginPoint().y);
        CHECK(res.EndPoint().x == expected.EndPoint().x);
        CHECK(res.EndPoint().y == expected.EndPoint().y);
    }

    SECTION("copy & shift no offset and CalculateShift") {
        Rect16 r, expected;
        ShiftDir_t dir;

        std::tie(r, dir, expected) = GENERATE(
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ -10, 10, 30, 30 }, ShiftDir_t::Left, { -40, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Left, { -20, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 40, 10, 30, 30 }, ShiftDir_t::Left, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ -10, 10, 30, 30 }, ShiftDir_t::Right, { 20, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Right, { 40, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ -40, 10, 30, 30 }, ShiftDir_t::Right, { -10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, -10, 30, 30 }, ShiftDir_t::Top, { 10, -40, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Top, { 10, -20, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, 40, 30, 30 }, ShiftDir_t::Top, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, -10, 30, 30 }, ShiftDir_t::Bottom, { 10, 20, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Bottom, { 10, 40, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, Rect16>({ 10, -40, 30, 30 }, ShiftDir_t::Bottom, { 10, -10, 30, 30 }));

        // internally use CalculateShift
        Rect16 res { r, dir };

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.BeginPoint().x == expected.BeginPoint().x);
        CHECK(res.BeginPoint().y == expected.BeginPoint().y);
        CHECK(res.EndPoint().x == expected.EndPoint().x);
        CHECK(res.EndPoint().y == expected.EndPoint().y);
    }
}

TEST_CASE("Swap") {
    Rect16 unSwapped = GENERATE(
        Rect16({ -10, 10, 30, 40 }),
        Rect16({ 10, 10, 30, 40 }),
        Rect16({ -10, 10, 30, 30 }),
        Rect16({ 0, 10, 30, 40 }),
        Rect16({ -10, 0, 40, 30 }));

    Rect16 swapped = unSwapped;
    swapped.SwapXY();

    CHECK(swapped.Width() == unSwapped.Height());
    CHECK(swapped.Height() == unSwapped.Width());
    CHECK(swapped.BeginPoint().x == unSwapped.BeginPoint().y);
    CHECK(swapped.BeginPoint().y == unSwapped.BeginPoint().x);
    CHECK(swapped.EndPoint().x == unSwapped.EndPoint().y);
    CHECK(swapped.EndPoint().y == unSwapped.EndPoint().x);
}

TEST_CASE("rectangle mirror", "[rectangle]") {
    Rect16 original, mirrored, swapped, swapped_mirrored;
    int16_t mirror_point;
    std::tie(original, mirror_point, mirrored) = GENERATE(
        std::make_tuple<Rect16, int16_t, Rect16>({ 0, 10, 30, 40 }, 0, { -30, 10, 30, 40 }),
        std::make_tuple<Rect16, int16_t, Rect16>({ 0, 10, 30, 40 }, 5, { -20, 10, 30, 40 }),
        std::make_tuple<Rect16, int16_t, Rect16>({ 0, 10, 30, 40 }, -5, { -40, 10, 30, 40 }),

        std::make_tuple<Rect16, int16_t, Rect16>({ 10, 10, 30, 40 }, 0, { -40, 10, 30, 40 }),
        std::make_tuple<Rect16, int16_t, Rect16>({ 10, 10, 30, 40 }, 5, { -30, 10, 30, 40 }),
        std::make_tuple<Rect16, int16_t, Rect16>({ 10, 10, 30, 40 }, -5, { -50, 10, 30, 40 }),

        std::make_tuple<Rect16, int16_t, Rect16>({ -10, 10, 30, 40 }, 0, { -20, 10, 30, 40 }),
        std::make_tuple<Rect16, int16_t, Rect16>({ -10, 10, 30, 40 }, 5, { -10, 10, 30, 40 }),
        std::make_tuple<Rect16, int16_t, Rect16>({ -10, 10, 30, 40 }, -5, { -30, 10, 30, 40 }),

        std::make_tuple<Rect16, int16_t, Rect16>({ 10, 10, 30, 40 }, 40, { 40, 10, 30, 40 }), // mirror at the end of rect
        std::make_tuple<Rect16, int16_t, Rect16>({ 10, 10, 30, 40 }, 100, { 160, 10, 30, 40 }),
        std::make_tuple<Rect16, int16_t, Rect16>({ -10, 10, 30, 40 }, 20, { 20, 10, 30, 40 }), // mirror at the end of rect
        std::make_tuple<Rect16, int16_t, Rect16>({ -10, 10, 30, 40 }, -100, { -190 - 30, 10, 30, 40 }));

    Rect16 res = original;
    res.MirrorX(mirror_point);
    CHECK(res == mirrored);

    Rect16 res_swapped = original;
    res_swapped.SwapXY();
    res_swapped.MirrorY(mirror_point);
    swapped_mirrored = mirrored;
    swapped_mirrored.SwapXY();
    CHECK(res_swapped == swapped_mirrored);
}

TEST_CASE("rectangle Contain point and IsEmpty", "[rectangle]") {
    Rect16 r;
    bool empty;
    std::tie(r, empty) = GENERATE(
        std::make_tuple<Rect16, bool>({ 0, 0, 30, 40 }, false),
        std::make_tuple<Rect16, bool>({ -30, -40, 30, 40 }, false), // ends 0,0
        std::make_tuple<Rect16, bool>({ 10, 10, 30, 40 }, false),
        std::make_tuple<Rect16, bool>({ -100, -100, 30, 40 }, false),
        std::make_tuple<Rect16, bool>({ 0, 0, 0, 0 }, true),
        std::make_tuple<Rect16, bool>({ 10, 10, 0, 10 }, true),
        std::make_tuple<Rect16, bool>({ 10, 10, 10, 0 }, true),
        std::make_tuple<Rect16, bool>({ 10, 10, 0, 0 }, true));

    // first must make sure IsEmptyWorks
    CHECK(r.IsEmpty() == empty);

    // empty does not contain anything
    CHECK_FALSE(r.Contain(r.TopLeft()) == r.IsEmpty());
    CHECK_FALSE(r.Contain(r.BottomRight()) == r.IsEmpty());
    CHECK_FALSE(r.Contain(r.EndPoint()));
    CHECK_FALSE(r.Contain(r.TopEndPoint()));
    CHECK_FALSE(r.Contain(r.LeftEndPoint()));
}

TEST_CASE("rectangle intersection", "[rectangle]") {
    Rect16 l, r, expected;
    std::tie(l, r, expected) = GENERATE(
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 20, 20, 40, 40 }, { 20, 20, 20, 20 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 20, 20, 40, 40 }, { 10, 10, 30, 30 }, { 20, 20, 20, 20 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 20, 0, 40, 40 }, { 20, 10, 20, 30 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 11, 10, 31, 20 }, { 11, 10, 29, 20 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 30, 30 }, { 1, 1, 29, 29 }, { 1, 1, 29, 29 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 0, 20, 30, 30 }, { 10, 0, 40, 22 }, { 10, 20, 20, 2 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 0, 20, 30, 30 }, { 0, 20, 30, 30 }, { 0, 20, 30, 30 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 20, 20 }, { 30, 30, 40, 40 }, { 0, 0, 0, 0 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 20, 20 }, { 15, 15, 0, 0 }, { 0, 0, 0, 0 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 0, 0 }, { 10, 10, 40, 40 }, { 0, 0, 0, 0 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 1, 10 }, { 10, 10, 10, 1 }, { 10, 10, 1, 1 }));

    Rect16 res = l.Intersection(r);

    CHECK(res.Width() == expected.Width());
    CHECK(res.Height() == expected.Height());
    CHECK(res.BeginPoint().x == expected.BeginPoint().x);
    CHECK(res.BeginPoint().y == expected.BeginPoint().y);
    CHECK(res.EndPoint().x == expected.EndPoint().x);
    CHECK(res.EndPoint().y == expected.EndPoint().y);
}

TEST_CASE("rectangles has intersection", "[rectangle]") {
    Rect16 l, r;
    bool expected;
    std::tie(l, r, expected) = GENERATE(
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 30, 30 }, { 20, 20, 40, 40 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 20, 20, 40, 40 }, { 10, 10, 30, 30 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 30, 30 }, { 20, 0, 40, 40 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 30, 30 }, { 11, 10, 31, 20 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 30, 30 }, { 1, 1, 29, 29 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 30 }, { 10, 0, 40, 22 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 20, 20 }, { 30, 30, 40, 40 }, false));

    CHECK(l.HasIntersection(r) == expected);
}

TEST_CASE("rectangles is subrectangle", "[rectangle]") {
    Rect16 l, r;
    bool expected;
    std::tie(l, r, expected) = GENERATE(
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 30, 30 }, { 20, 20, 40, 40 }, false),
        std::make_tuple<Rect16, Rect16, bool>({ 20, 20, 40, 40 }, { 10, 10, 30, 30 }, false),
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 30, 30 }, { 0, 0, 40, 40 }, false),
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 30, 30 }, { 11, 10, 31, 20 }, false),
        std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 30, 30 }, { 1, 1, 29, 29 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 30 }, { 10, 0, 40, 22 }, false),
        std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 30 }, { 0, 20, 30, 30 }, true),
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 20, 20 }, { 20, 20, 40, 40 }, false));

    CHECK(l.Contain(r) == expected);
}

TEST_CASE("rectangle point arithmetic", "[rectangle]") {
    SECTION("operator=") {
        point_i16_t point = GENERATE(point_i16_t({ 0, 0 }), point_i16_t({ 10, 10 }), point_i16_t({ -2, 8 }), point_i16_t({ -33, 0 }));
        Rect16 r = GENERATE( // this operation does not have meaning on empty rect -  must not be empty
            Rect16({ 0, 0, 1, 1 }),
            Rect16({ 10, 10, 5, 8 }),
            Rect16({ -2, 0, 3, 2 }));
        r = point;
        CHECK(r.TopLeft() == point);
    }

    SECTION("operator+") {
        // it use internally +=
        Rect16 r;
        point_i16_t point, expected;
        std::tie(r, point, expected) = GENERATE(
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ 0, 0, 30, 30 }, { 0, 2 }, { 0, 2 }),
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ 0, 20, 30, 30 }, { 6, -5 }, { 6, 15 }),
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ -5, 20, 30, 30 }, { -3, -30 }, { -8, -10 }),
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ -6, -1, 30, 30 }, { 20, 20 }, { 14, 19 }));
        CHECK((r + point).TopLeft() == expected);
    }

    SECTION("operator-") {
        // it use internally -=
        Rect16 r;
        point_i16_t point, expected;
        std::tie(r, point, expected) = GENERATE(
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ 0, 0, 30, 30 }, { 0, 2 }, { 0, -2 }),
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ 0, 20, 30, 30 }, { 6, -5 }, { -6, 25 }),
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ -5, 20, 30, 30 }, { -3, -30 }, { -2, 50 }),
            std::make_tuple<Rect16, point_i16_t, point_i16_t>({ -6, -1, 30, 30 }, { 20, 20 }, { -26, -21 }));
        CHECK((r - point).TopLeft() == expected);
    }

    SECTION("operators == and !=") {
        // it use internally -=
        Rect16 r0, r1;
        bool equal;

        std::tie(r0, r1, equal) = GENERATE(
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, true),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 30, 30 }, { 0, 0, 30, 30 }, true),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 30 }, { 0, 20, 30, 30 }, true),
            std::make_tuple<Rect16, Rect16, bool>({ -5, 20, 30, 30 }, { -5, 20, 30, 30 }, true),
            std::make_tuple<Rect16, Rect16, bool>({ -6, -1, 30, 30 }, { -6, -1, 30, 30 }, true),

            // x is wrong
            std::make_tuple<Rect16, Rect16, bool>({ 1, 0, 0, 0 }, { 0, 0, 0, 0 }, true), // all empty rectangles are equal
            std::make_tuple<Rect16, Rect16, bool>({ 22, 0, 30, 30 }, { 0, 0, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ 89, 20, 30, 30 }, { 0, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 30 }, { -5, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -4, -1, 30, 30 }, { -6, -1, 30, 30 }, false),

            // y is wrong
            std::make_tuple<Rect16, Rect16, bool>({ 0, -20, 0, 0 }, { 0, 0, 0, 0 }, true), // all empty rectangles are equal
            std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 30 }, { 0, 0, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 30, 30 }, { 0, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -5, 0, 30, 30 }, { -5, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -6, -21, 30, 30 }, { -6, -1, 30, 30 }, false),

            // w is wrong
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 10, 0 }, { 0, 0, 0, 0 }, true), // all empty rectangles are equal
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 0, 30 }, { 0, 0, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 10, 30 }, { 0, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -5, 20, 300, 30 }, { -5, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -6, -1, 0, 30 }, { -6, -1, 30, 30 }, false),

            // h is wrong
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 0, 110 }, { 0, 0, 0, 0 }, true), // all empty rectangles are equal
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 30, 0 }, { 0, 0, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 3 }, { 0, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -5, 20, 30, 322 }, { -5, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -6, -1, 30, 1 }, { -6, -1, 30, 30 }, false),

            // multiple wrong values
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 3, 0 }, { 0, 0, 0, 1 }, true), // all empty rectangles are equal
            std::make_tuple<Rect16, Rect16, bool>({ 1, 1, 1, 1 }, { 0, 0, 0, 0 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 0, 3, 6 }, { 0, 0, 0, 0 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -3, -3, 30, 30 }, { 0, 0, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 3, 3 }, { 0, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -5, 2, 3, 3 }, { -5, 20, 30, 30 }, false),
            std::make_tuple<Rect16, Rect16, bool>({ -60, -10, 3, 30 }, { -6, -1, 30, 30 }, false));

        CHECK((r0 == r1) == equal);
        CHECK((r0 != r1) != equal);
    }
}

TEST_CASE("rectangle LimitSize", "[rectangle]") {
    SECTION("not empty") {
        Rect16 r, expected;
        size_ui16_t limit;
        std::tie(r, limit, expected) = GENERATE(
            std::make_tuple<Rect16, size_ui16_t, Rect16>({ 0, 0, 30, 30 }, { 2, 2 }, { 0, 0, 2, 2 }),
            std::make_tuple<Rect16, size_ui16_t, Rect16>({ 0, 20, 80, 40 }, { 6, 1000 }, { 0, 20, 6, 40 }),
            std::make_tuple<Rect16, size_ui16_t, Rect16>({ -5, 20, 20, 1 }, { 20, 1 }, { -5, 20, 20, 1 }),
            std::make_tuple<Rect16, size_ui16_t, Rect16>({ -6, -1, 100, 3 }, { 99, 3 }, { -6, -1, 99, 3 }));

        Rect16 r_sw = r;
        size_ui16_t limit_sw = { limit.h, limit.w };
        Rect16 expected_sw = expected;
        r_sw.SwapXY();
        expected_sw.SwapXY();

        r.LimitSize(limit);
        CHECK(r == expected);

        r_sw.LimitSize(limit_sw);
        CHECK(r_sw == expected_sw);
    }
    SECTION("empty") {
        Rect16 r;
        size_ui16_t limit;
        std::tie(r, limit) = GENERATE(
            std::make_tuple<Rect16, size_ui16_t>({ 0, 0, 0, 0 }, { 0, 0 }),
            std::make_tuple<Rect16, size_ui16_t>({ 0, 0, 0, 30 }, { 2, 2 }),
            std::make_tuple<Rect16, size_ui16_t>({ 0, 20, 80, 40 }, { 6, 0 }),
            std::make_tuple<Rect16, size_ui16_t>({ -5, 20, 0, 0 }, { 20, 1 }),
            std::make_tuple<Rect16, size_ui16_t>({ -6, -1, 100, 3 }, { 0, 0 }));

        r.LimitSize(limit);
        CHECK(r.IsEmpty());
    }
}

TEST_CASE("rectangle Transform", "[rectangle]") {
    SECTION("not empty") {
        Rect16 r, target, expected;
        std::tie(r, target, expected) = GENERATE(
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 30, 30 }, { 2, 3, 100, 100 }, { 2, 3, 30, 30 }), // fits
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 2, 80, 40 }, { 1, 2, 5, 5 }, { 1, 4, 5, 3 }), // does not fit
            std::make_tuple<Rect16, Rect16, Rect16>({ 5, 20, 20, 1 }, { 20, -3, 6, 200 }, { 25, 17, 1, 1 }), // width does not fit
            std::make_tuple<Rect16, Rect16, Rect16>({ 10, 1, 100, 3 }, { -100, 3, 1000, 2 }, { -90, 4, 100, 1 }), // height does not fit
            // rect with negative coords is cut
            // data for X, Y is made by SwapXY
            std::make_tuple<Rect16, Rect16, Rect16>({ -1, 0, 30, 30 }, { 2, 3, 100, 100 }, { 2, 3, 29, 30 }), // negative x
            std::make_tuple<Rect16, Rect16, Rect16>({ -1, 8, 30, 30 }, { 2, 3, 10, 100 }, { 2, 11, 10, 30 }), // negative x, does not fit into target
            std::make_tuple<Rect16, Rect16, Rect16>({ -22, 4, 30, 30 }, { 2, 3, 10, 100 }, { 2, 7, 8, 30 }), // negative x, would not fit into target, but fits after negative coord cut
            std::make_tuple<Rect16, Rect16, Rect16>({ -22, 2, 30, 30 }, { 2, 3, 1, 100 }, { 2, 5, 1, 30 }), // negative x, would not fit into target, and still does not fit even after negative coord cut
            // both X and Y negative
            std::make_tuple<Rect16, Rect16, Rect16>({ -1, -1, 10, 6 }, { 2, 3, 100, 100 }, { 2, 3, 9, 5 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ -1, -4, 20, 7 }, { 2, 3, 10, 100 }, { 2, 3, 10, 3 }), // X does not fit into target
            std::make_tuple<Rect16, Rect16, Rect16>({ -22, -2, 30, 8 }, { 2, 3, 10, 100 }, { 2, 3, 8, 6 }) // X would not fit into target, but fits after negative coord cut
        );

        Rect16 r_sw = r;
        Rect16 target_sw = target;
        Rect16 expected_sw = expected;
        r_sw.SwapXY();
        target_sw.SwapXY();
        expected_sw.SwapXY();

        r.Transform(target);
        CHECK(r == expected);

        r_sw.Transform(target_sw);
        CHECK(r_sw == expected_sw);
    }
}

TEST_CASE("rectangle union", "[rectangle]") {
    SECTION("single rectangle") {
        // it also tests operators + and += since Union use them
        Rect16 l, r, expected;
        std::tie(l, r, expected) = GENERATE(
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 20, 20 }, { 20, 20, 40, 40 }, { 0, 0, 60, 60 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 40, 40 }, { 20, 20, 20, 20 }, { 0, 0, 40, 40 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 20, 20, 10, 10 }, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ -21, -22, 10, 10 }, { 0, 0, 25, 30 }, { -21, -22, 46, 52 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ -20, -20, 10, 10 }, { -40, -40, 10, 10 }, { -40, -40, 30, 30 }));

        Rect16 res = l.Union(r);

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.BeginPoint().x == expected.BeginPoint().x);
        CHECK(res.BeginPoint().y == expected.BeginPoint().y);
        CHECK(res.EndPoint().x == expected.EndPoint().x);
        CHECK(res.EndPoint().y == expected.EndPoint().y);
    }

    SECTION("sequence") {
        using Sequence = std::array<Rect16, 8>;
        Sequence s;
        Rect16 l, expected;

        std::tie(l, s, expected) = GENERATE(
            std::make_tuple<Rect16, Sequence, Rect16>({ 0, 0, 10, 10 }, { {} }, { 0, 0, 10, 10 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ 0, 0, 10, 10 }, { { {} } }, { 0, 0, 10, 10 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ 0, 0, 20, 20 }, { { { 20, 20, 40, 40 } } }, { 0, 0, 60, 60 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ 0, 0, 20, 20 }, { { { 0, 20, 20, 40 }, { 20, 0, 40, 20 } } }, { 0, 0, 60, 60 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ 10, 10, 20, 20 }, { { { 0, 0, 10, 10 }, { 0, 20, 20, 40 }, { 20, 0, 40, 20 } } }, { 0, 0, 60, 60 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ -21, -22, 10, 10 }, { { { 0, 0, 25, 30 } } }, { -21, -22, 46, 52 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ -20, -20, 10, 10 }, { { { -40, -40, 10, 10 } } }, { -40, -40, 30, 30 }));

        Rect16 res = l.Union(s);

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.BeginPoint().x == expected.BeginPoint().x);
        CHECK(res.BeginPoint().y == expected.BeginPoint().y);
        CHECK(res.EndPoint().x == expected.EndPoint().x);
        CHECK(res.EndPoint().y == expected.EndPoint().y);
    }
}

TEST_CASE("rectangle Align", "[rectangle]") {
    Rect16 toBeAligned, alignRC;
    Align_t align = Align_t::Center();
    point_i16_t expected_point;
    size_ui16_t sz;
    SECTION("precise fit") {
        sz = { 25, 52 }; // precise fit, all rects has same size

        std::tie(toBeAligned, alignRC, align) = std::make_tuple<Rect16, Rect16, Align_t>(
            Rect16(
                GENERATE(point_i16_t({ 0, 0 }), point_i16_t({ -10, 30 }), point_i16_t({ 110, 0 })) // some X Y coords
                ,
                sz),
            Rect16(
                GENERATE(point_i16_t({ 0, 0 }), point_i16_t({ 10, -30 }), point_i16_t({ 333, 222 })) // some X Y coords
                ,
                sz),
            Align_t(
                GENERATE(Align_t(Align_t::vertical::top), Align_t(Align_t::horizontal::center), Align_t(Align_t::vertical::center, Align_t::horizontal::center))) // precise fit, align should not matter
        );

        toBeAligned.Align(alignRC, align);

        CHECK(toBeAligned == alignRC); // alignRC precisely fits in toBeAligned
    }

    SECTION("normal use") {
        std::tie(toBeAligned, alignRC, align, expected_point) = GENERATE(
            std::make_tuple<Rect16, Rect16, Align_t, point_i16_t>({ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, Align_t(Align_t::vertical::top, Align_t::horizontal::left), { 0, 0 }), // zero aligned via zero aligns to zero
            std::make_tuple<Rect16, Rect16, Align_t, point_i16_t>({ 0, 0, 10, 20 }, { 3, 5, 100, 100 }, Align_t(Align_t::vertical::top, Align_t::horizontal::left), { 3, 5 }),
            std::make_tuple<Rect16, Rect16, Align_t, point_i16_t>({ 666, 0, 10, 20 }, { 0, 0, 100, 100 }, Align_t(Align_t::vertical::top, Align_t::horizontal::left), { 0, 0 }),
            std::make_tuple<Rect16, Rect16, Align_t, point_i16_t>({ 0, 666, 3, 5 }, { 1, -1, 5, 9 }, Align_t(Align_t::vertical::center, Align_t::horizontal::center), { 2, 1 }),
            std::make_tuple<Rect16, Rect16, Align_t, point_i16_t>({ 0, 0, 100, 20 }, { 0, 0, 10, 10 }, Align_t(Align_t::vertical::top, Align_t::horizontal::left), { 0, 0 }), // does not fit .. should not matter for top left
            std::make_tuple<Rect16, Rect16, Align_t, point_i16_t>({ 666, 0, 30, 20 }, { 0, 0, 10, 10 }, Align_t(Align_t::vertical::bottom, Align_t::horizontal::right), { -20, -10 }));

        sz = toBeAligned.Size();
        toBeAligned.Align(alignRC, align);

        CHECK(toBeAligned == Rect16(expected_point, sz));
    }
}

TEST_CASE("rectangle add padding", "[rectangle]") {
    Rect16 l, expected;
    padding_ui8_t p;

    std::tie(l, p, expected) = GENERATE(
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 20, 20 }, { 0, 0, 0, 0 }, { 0, 0, 20, 20 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 40, 40 }, { 20, 10, 30, 40 }, { -20, -10, 90, 90 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 20, 0, 40, 0 }, { -10, 10, 90, 30 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 20, 0, 0, 0 }, { -10, 10, 50, 30 }));

    l.AddPadding(p);

    CHECK(l.Width() == expected.Width());
    CHECK(l.Height() == expected.Height());
    CHECK(l.BeginPoint().x == expected.BeginPoint().x);
    CHECK(l.BeginPoint().y == expected.BeginPoint().y);
    CHECK(l.EndPoint().x == expected.EndPoint().x);
    CHECK(l.EndPoint().y == expected.EndPoint().y);
}

TEST_CASE("rectangle cut padding", "[rectangle]") {
    Rect16 l, expected;
    padding_ui8_t p;

    std::tie(l, p, expected) = GENERATE(
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 20, 20 }, { 0, 0, 0, 0 }, { 0, 0, 20, 20 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 40, 40 }, { 20, 10, 30, 40 }, { 0, 0, 0, 0 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 40, 30 }, { 10, 0, 10, 0 }, { 20, 10, 20, 30 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 10, 0, 10, 0 }, { 20, 10, 10, 30 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 0, 10, 0, 10 }, { 10, 20, 30, 10 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 70, 70 }, { 10, 10, 20, 30 }, { 20, 20, 40, 30 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 40, 30 }, { 20, 0, 0, 0 }, { 30, 10, 20, 30 }));

    l.CutPadding(p);

    CHECK(l.Width() == expected.Width());
    CHECK(l.Height() == expected.Height());
    CHECK(l.BeginPoint().x == expected.BeginPoint().x);
    CHECK(l.BeginPoint().y == expected.BeginPoint().y);
    CHECK(l.EndPoint().x == expected.EndPoint().x);
    CHECK(l.EndPoint().y == expected.EndPoint().y);
}

TEST_CASE("rectangle Merge", "[rectangle]") {
    SECTION("static impl") {
        using Sequence = std::array<Rect16, 8>;
        Sequence s;
        Rect16 expected;

        std::tie(s, expected) = GENERATE(
            std::make_tuple<Sequence, Rect16>({ {} }, { 0, 0, 0, 0 }),
            std::make_tuple<Sequence, Rect16>({ { { 0, 0, 20, 20 },
                                                  { 20, 20, 40, 40 } } },
                { 0, 0, 60, 60 }),
            std::make_tuple<Sequence, Rect16>({ { { 0, 0, 20, 20 },
                                                  { 0, 20, 20, 40 },
                                                  { 20, 0, 40, 20 } } },
                { 0, 0, 60, 60 }),
            std::make_tuple<Sequence, Rect16>({ { { 10, 10, 20, 20 },
                                                  { 0, 0, 10, 10 },
                                                  { 0, 20, 20, 40 },
                                                  { 20, 0, 40, 20 } } },
                { 0, 0, 60, 60 }),
            std::make_tuple<Sequence, Rect16>({ { { -20, -20, 10, 10 },
                                                  { 0, 0, 20, 20 } } },
                { -20, -20, 40, 40 }));

        Rect16 res = Rect16::Merge(s);

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.BeginPoint().x == expected.BeginPoint().x);
        CHECK(res.BeginPoint().y == expected.BeginPoint().y);
        CHECK(res.EndPoint().x == expected.EndPoint().x);
        CHECK(res.EndPoint().y == expected.EndPoint().y);
    }
}

TEST_CASE("rectangle Merge_ParamPack", "[rectangle]") {
    SECTION("static impl") {
        Rect16 res;
        Rect16 expected;

        std::tie(res, expected) = GENERATE(
            std::make_tuple<Rect16, Rect16>(Rect16::Merge_ParamPack(Rect16()), { 0, 0, 0, 0 }),
            std::make_tuple<Rect16, Rect16>(Rect16::Merge_ParamPack(Rect16(0, 0, 20, 20), Rect16(20, 20, 40, 40)), { 0, 0, 60, 60 }),
            std::make_tuple<Rect16, Rect16>(Rect16::Merge_ParamPack(Rect16(0, 0, 20, 20), Rect16(0, 20, 20, 40), Rect16(20, 0, 40, 20)),
                { 0, 0, 60, 60 }));

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.BeginPoint().x == expected.BeginPoint().x);
        CHECK(res.BeginPoint().y == expected.BeginPoint().y);
        CHECK(res.EndPoint().x == expected.EndPoint().x);
        CHECK(res.EndPoint().y == expected.EndPoint().y);
    }
}

TEST_CASE("rectangle Contain rectangle", "[rectangle]") {
    Rect16 r;
    bool expected;
    point_i16_t p;

    std::tie(r, p, expected) = GENERATE(
        std::make_tuple<Rect16, point_i16_t, bool>({ 0, 0, 10, 10 }, { 20, 20 }, false),
        std::make_tuple<Rect16, point_i16_t, bool>({ 0, 0, 10, 10 }, { 0, 0 }, true),
        std::make_tuple<Rect16, point_i16_t, bool>({ 0, 0, 10, 10 }, { 5, 5 }, true));

    bool res = r.Contain(p);

    CHECK(res == expected);
}

TEST_CASE("rectangle split", "[rectangle]") {
    using Sequence = std::array<Rect16, 4>;
    using Ratio = std::array<uint8_t, 4>;

    SECTION("horizontal - splits with spaces") {
        Sequence expSplits, expSpaces;
        Rect16 r;
        size_t count;
        uint16_t spacing;
        Ratio ratio;
        Rect16 splits[4];
        Rect16 spaces[4];

        // TESTING
        //  r = Rect16({0, 0}, 120, 100);
        //  count = 4;
        //  spacing = 10;
        //  ratio = {1, 2, 2, 1};
        //
        //  r.HorizontalSplit(splits, spaces, count, spacing, ratio.data());
        //
        //  CHECK(spaces[0].TopLeft().x == 15);
        //  CHECK(spaces[1].TopLeft().x == 55);
        //  CHECK(spaces[2].TopLeft().x == 95);

        std::tie(r, count, spacing, ratio, expSplits, expSpaces) = GENERATE(
            std::make_tuple<Rect16, size_t, uint16_t, Ratio, Sequence, Sequence>(
                { 0, 0, 100, 100 }, 2, 0, { { 20, 20 } }, { { { 0, 0, 50, 100 }, { 50, 0, 50, 100 } } }, { { { 50, 0, 0, 100 } } }),
            std::make_tuple<Rect16, size_t, uint16_t, Ratio, Sequence, Sequence>(
                { 0, 0, 120, 100 }, 4, 10, { { 10, 15, 15, 10 } }, { { { 0, 0, 20, 100 }, { 30, 0, 25, 100 }, { 65, 0, 25, 100 }, { 100, 0, 20, 100 } } }, { { { 20, 0, 10, 100 }, { 55, 0, 10, 100 }, { 90, 0, 10, 100 } } }));

        r.HorizontalSplit(splits, spaces, count, spacing, ratio.data());

        COMPARE_ARRAYS(splits, expSplits);
        COMPARE_ARRAYS(spaces, expSpaces);
    }

    SECTION("horizontal - given widths 'footer style'") {

        Rect16 r;
        static constexpr size_t max_count = 4;
        static constexpr size_t h = 10;

        std::vector<Rect16> expSplits; // expected
        std::array<Rect16, max_count> splits;
        std::vector<Rect16::Width_t> widths;

        std::tie(r, widths, expSplits) = GENERATE(
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 0, 0, 100, h }, { 1, 2 }, { { { 0, 0, 1, h }, { 98, 0, 2, h } } }),
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 0, 0, 7, h }, { 1, 1, 1, 1 }, { { { 0, 0, 1, h }, { 2, 0, 1, h }, { 4, 0, 1, h }, { 6, 0, 1, h } } }),
            // last does not fit
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 0, 0, 12, h }, { 1, 2, 3, 40 }, { { { 0, 0, 1, h }, { 4, 0, 2, h }, { 9, 0, 3, h } } }),
            // not exact space, last is bit further
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 0, 0, 8, h }, { 1, 1, 1, 1 }, { { { 0, 0, 1, h }, { 2, 0, 1, h }, { 4, 0, 1, h }, { 7, 0, 1, h } } }),
            // only one fits
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 1, 2, 10, h }, { 5, 100, 1, 1 }, { { { 1, 2, 5, h } } }),
            // 2 border empty rects .. any empty rects are equal
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 1, 2, 14, h }, { 0, 2, 3, 0 }, { { { 0, 0, 0, 0 }, { 4, 2, 2, h }, { 9, 2, 3, h }, { 0, 0, 0, 0 } } }),
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 0, 2, 9, h }, { 0, 5, 0 }, { { { 0, 0, 0, 0 }, { 2, 2, 5, h }, { 0, 0, 0, 0 } } }),
            // empty
            std::make_tuple<Rect16, std::vector<Rect16::Width_t>, std::vector<Rect16>>(
                { 1, 0, 10, h }, { 50, 10, 1, 1 }, std::vector<Rect16>()));

        size_t expCount = expSplits.size();
        size_t do_N_splits = widths.size();
        CHECK(expCount <= max_count);
        CHECK(do_N_splits <= max_count); // unnecessary, next check would find it too, but this will tell me exact reason of failure
        CHECK(expCount <= do_N_splits);

        size_t count = r.HorizontalSplit(&splits[0], &widths[0], do_N_splits);
        CHECK(expCount == count);
        COMPARE_ARRAYS_SIZE_FROM_SMALLER(splits, expSplits);
    }

    SECTION("horizontal - cuts") {
        Sequence expected, result;
        Rect16 r;
        uint16_t span, count;

        std::tie(r, span, count, expected) = GENERATE(
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 0, 0 }, 10, 0, { { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 10, 4, { { { 0, 0, 10, 100 }, { 10, 0, 10, 100 }, { 20, 0, 10, 100 }, { 30, 0, 10, 100 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 30, 3, { { { 0, 0, 30, 100 }, { 30, 0, 30, 100 }, { 60, 0, 30, 100 } } }));

        size_t l = r.HorizontalSplit(result, span);
        CHECK(l == count);
        COMPARE_ARRAYS(expected, result);
    }

    SECTION("vertical - cuts") {
        Sequence expected, result;
        Rect16 r;
        uint16_t span, count;

        std::tie(r, span, count, expected) = GENERATE(
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 0, 0 }, 10, 0, { { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 10, 4, { { { 0, 0, 100, 10 }, { 0, 10, 100, 10 }, { 0, 20, 100, 10 }, { 0, 30, 100, 10 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 30, 3, { { { 0, 0, 100, 30 }, { 0, 30, 100, 30 }, { 0, 60, 100, 30 } } })

        );

        size_t l = r.VerticalSplit(result, span);
        CHECK(l == count);
        COMPARE_ARRAYS(expected, result);
    }
}

TEST_CASE("rectangle LeftSubrect", "[rectangle]") {
    SECTION("empty") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 0, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("normal cut") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 4, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 4, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("cut till end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 8, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 8, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("cut behind end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 12, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 12, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("empty cut at end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 16, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = minuend;

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("empty cut behind end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 32, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = minuend;

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("empty cut in front begin") {
        // y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }
}

TEST_CASE("rectangle RightSubrect", "[rectangle]") {
    SECTION("normal cut from begin") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 8, 1, 8, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("normal cut middle") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 4, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 12, 1, 4, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("cut till end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 8, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("cut behind end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 12, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("empty cut at end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 16, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("empty cut behind end") {
        // y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 32, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("not intersecting cut in front begin") {
        // y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = { 8, 1, 16, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("intersecting cut in front begin") {
        // y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 16, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = { 16, 1, 8, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("overlaping cut") {
        // y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 }; // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 32, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }
}
