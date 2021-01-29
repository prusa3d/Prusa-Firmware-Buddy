#include "catch2/catch.hpp"

#include "guiapi/include/Rect16.h"

#include <vector>
#include <tuple>

/// Warning!
/// With yet unknown reason for us the method Catch::getResultCapture() returns nullptr in case the
/// benchmarks in CATCH2 are configured as enable. Please consider this issue when you'll decide to
/// write benchmark tests.
#define COMPARE_ARRAYS(lhs, rhs) compareArrays(Catch::getResultCapture().getCurrentTestName(), __LINE__, lhs, rhs)

template <typename T, size_t N>
void compareArrays(const std::string &test, unsigned line, std::array<T, N> lhs, std::array<T, N> rhs) {
    std::vector<T> lv(lhs.begin(), lhs.end());
    std::vector<T> rv(rhs.begin(), rhs.end());
    INFO("Test case [" << test << "] failed at line " << line); // Reported only if REQUIRE fails
    CHECK(lv == rv);
}

template <typename T, size_t N>
void compareArrays(const std::string &test, unsigned line, T *lhs, std::array<T, N> rhs) {
    std::vector<T> lv(lhs, lhs + N);
    std::vector<T> rv(rhs.begin(), rhs.end());
    INFO("Test case [" << test << "] failed at line " << line); // Reported only if REQUIRE fails
    CHECK(lv == rv);
}

TEST_CASE("rectangle construc", "[rectangle]") {

    SECTION("topleft corner & width & height") {
        point_i16_t top_left = { 10, 20 };
        size_ui16_t size = { 20, 40 };
        Rect16 r { top_left, size };
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
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 20, 20 }, { 30, 30, 40, 40 }, { 0, 0, 0, 0 }));

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

TEST_CASE("rectangle union", "[rectangle]") {
    SECTION("single rectangle") {
        Rect16 l, r, expected;
        std::tie(l, r, expected) = GENERATE(
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 20, 20 }, { 20, 20, 40, 40 }, { 0, 0, 60, 60 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 40, 40 }, { 20, 20, 20, 20 }, { 0, 0, 40, 40 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 20, 20, 10, 10 }, { 10, 10, 30, 30 }));

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
            std::make_tuple<Rect16, Sequence, Rect16>({ -20, -20, 10, 10 }, { { { 0, 0, 20, 20 } } }, { -20, -20, 20, 20 }));

        Rect16 res = l.Union(s);

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.BeginPoint().x == expected.BeginPoint().x);
        CHECK(res.BeginPoint().y == expected.BeginPoint().y);
        CHECK(res.EndPoint().x == expected.EndPoint().x);
        CHECK(res.EndPoint().y == expected.EndPoint().y);
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

TEST_CASE("rectangle Contain", "[rectangle]") {
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

        //TESTING
        // r = Rect16({0, 0}, 120, 100);
        // count = 4;
        // spacing = 10;
        // ratio = {1, 2, 2, 1};
        //
        // r.HorizontalSplit(splits, spaces, count, spacing, ratio.data());
        //
        // CHECK(spaces[0].TopLeft().x == 15);
        // CHECK(spaces[1].TopLeft().x == 55);
        // CHECK(spaces[2].TopLeft().x == 95);

        std::tie(r, count, spacing, ratio, expSplits, expSpaces) = GENERATE(
            std::make_tuple<Rect16, size_t, uint16_t, Ratio, Sequence, Sequence>(
                { 0, 0, 100, 100 }, 2, 0, { { 1, 1 } }, { { { 0, 0, 50, 100 }, { 50, 0, 50, 100 } } }, { { { 50, 0, 0, 100 } } }),
            std::make_tuple<Rect16, size_t, uint16_t, Ratio, Sequence, Sequence>(
                { 0, 0, 120, 100 }, 4, 10, { { 1, 2, 2, 1 } }, { { { 0, 0, 15, 100 }, { 25, 0, 30, 100 }, { 65, 0, 30, 100 }, { 105, 0, 15, 100 } } }, { { { 15, 0, 10, 100 }, { 55, 0, 10, 100 }, { 95, 0, 10, 100 } } }));

        r.HorizontalSplit(splits, spaces, count, spacing, ratio.data());

        COMPARE_ARRAYS(splits, expSplits);
        COMPARE_ARRAYS(spaces, expSpaces);
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
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 0, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("normal cut") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 4, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 4, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("cut till end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 8, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 8, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("cut behind end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 12, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 0, 1, 12, 1 };

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("empty cut at end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 16, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = minuend;

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("empty cut behind end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 32, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = minuend;

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("empty cut in front begin") {
        //y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.LeftSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }
}

TEST_CASE("rectangle RightSubrect", "[rectangle]") {
    SECTION("normal cut from begin") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 8, 1, 8, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("normal cut middle") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 4, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result { 12, 1, 4, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("cut till end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 8, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("cut behind end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 12, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("empty cut at end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 16, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("empty cut behind end") {
        //y and h does not matter
        Rect16 minuend { 0, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 32, 1, 8, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }

    SECTION("not intersecting cut in front begin") {
        //y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 };   // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 8, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = { 8, 1, 16, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("intersecting cut in front begin") {
        //y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 16, 1 }; // the rect that is to be subtracted.
        Rect16 expected_result = { 16, 1, 8, 1 };

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(expected_result == result);
    }

    SECTION("overlaping cut") {
        //y and h does not matter
        Rect16 minuend { 8, 1, 16, 1 };    // the rect that is to be subtracted from.
        Rect16 subtrahend { 0, 1, 32, 1 }; // the rect that is to be subtracted.

        Rect16 result = minuend.RightSubrect(subtrahend);

        REQUIRE(result.Width() == 0);
    }
}
