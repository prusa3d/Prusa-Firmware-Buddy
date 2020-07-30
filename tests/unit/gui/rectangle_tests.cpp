#include "catch2/catch.hpp"

#include "guiapi/include/Rect16.h"

#include <vector>
#include <tuple>

#define COMPARE_ARRAYS(lhs, rhs) compareArrays(Catch::getResultCapture().getCurrentTestName(), __LINE__, lhs, rhs)

template <typename T, size_t N>
void compareArrays(const std::string &test, unsigned line, std::array<T, N> lhs, std::array<T, N> rhs) {
    std::vector<T> lv(lhs.begin(), lhs.end());
    std::vector<T> rv(rhs.begin(), rhs.end());
    INFO("Test case [" << test << "] failed at line " << line); // Reported only if REQUIRE fails
    CHECK(lv == rv);
}

TEST_CASE("rectangle construc", "[rectangle]") {

    SECTION("topleft corner & width & height") {
        point_i16_t top_left = { 10, 20 };
        Rect16 r { top_left, 20, 10 };
        CHECK(r.Width() == 20);
        CHECK(r.Height() == 10);
    }

    SECTION("topleft corner & size") {
        point_i16_t top_left = { 10, 20 };
        size_ui16_t size = { 20, 40 };
        Rect16 r { top_left, size };
        CHECK(r.BottomRight().x == 30);
        CHECK(r.BottomRight().y == 60);
    }

    SECTION("empty box") {
        Rect16 r;
        CHECK(r.Width() == 0);
        CHECK(r.Height() == 0);
    }

    SECTION("by coordinates") {
        Rect16 r { 10, 20, 20, 40 };
        CHECK(r.Width() == 10);
        CHECK(r.Height() == 20);
    }

    SECTION("copy construct") {
        Rect16 q { 10, 20, 20, 40 };
        Rect16 r { q };
        CHECK(r.Width() == 10);
        CHECK(r.Height() == 20);
    }

    SECTION("by coordinates & wrong x") {
        Rect16 r { 10, 20, 0, 40 };
        CHECK(r.Width() == 0);
        CHECK(r.Height() == 20);
    }

    SECTION("by coordinates & wrong y") {
        Rect16 r { 10, 20, 20, 10 };
        CHECK(r.Width() == 10);
        CHECK(r.Height() == 0);
    }

    SECTION("copy & shift") {
        Rect16 r, expected;
        ShiftDir_t dir;
        uint16_t offset;

        std::tie(r, dir, offset, expected) = GENERATE(
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Left, 20, { -10, 10, 10, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Left, 10, { 0, 10, 20, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Left, 0, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Right, 20, { 30, 10, 50, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Right, 10, { 20, 10, 40, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Right, 0, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Top, 20, { 10, -10, 30, 10 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Top, 10, { 10, 0, 30, 20 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Top, 0, { 10, 10, 30, 30 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Bottom, 20, { 10, 30, 30, 50 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Bottom, 10, { 10, 20, 30, 40 }),
            std::make_tuple<Rect16, ShiftDir_t, uint16_t, Rect16>({ 10, 10, 30, 30 }, ShiftDir_t::Bottom, 0, { 10, 10, 30, 30 }));

        Rect16 res { r, dir, offset };

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.TopLeft().x == expected.TopLeft().x);
        CHECK(res.TopLeft().y == expected.TopLeft().y);
        CHECK(res.BottomRight().x == expected.BottomRight().x);
        CHECK(res.BottomRight().y == expected.BottomRight().y);
    }
}

TEST_CASE("rectangle intersection", "[rectangle]") {

    Rect16 l, r, expected;
    std::tie(l, r, expected) = GENERATE(
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 20, 20, 40, 40 }, { 20, 20, 30, 30 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 20, 20, 40, 40 }, { 10, 10, 30, 30 }, { 20, 20, 30, 30 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 20, 0, 40, 40 }, { 20, 10, 30, 30 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 11, 10, 31, 20 }, { 11, 10, 30, 20 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 30, 30 }, { 1, 1, 29, 29 }, { 1, 1, 29, 29 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 0, 20, 30, 30 }, { 10, 0, 40, 22 }, { 10, 20, 30, 22 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 0, 20, 30, 30 }, { 0, 20, 30, 30 }, { 0, 20, 30, 30 }),
        std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 20, 20 }, { 20, 20, 40, 40 }, { 0, 0, 0, 0 }));

    Rect16 res = l.Intersection(r);

    CHECK(res.Width() == expected.Width());
    CHECK(res.Height() == expected.Height());
    CHECK(res.TopLeft().x == expected.TopLeft().x);
    CHECK(res.TopLeft().y == expected.TopLeft().y);
    CHECK(res.BottomRight().x == expected.BottomRight().x);
    CHECK(res.BottomRight().y == expected.BottomRight().y);
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
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 20, 20 }, { 20, 20, 40, 40 }, false));

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
        std::make_tuple<Rect16, Rect16, bool>({ 0, 20, 30, 30 }, { 0, 20, 30, 30 }, false),
        std::make_tuple<Rect16, Rect16, bool>({ 10, 10, 20, 20 }, { 20, 20, 40, 40 }, false));

    CHECK(l.IsSubrectangle(r) == expected);
}

TEST_CASE("rectangle union", "[rectangle]") {
    SECTION("single rectangle") {
        Rect16 l, r, expected;
        std::tie(l, r, expected) = GENERATE(
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 20, 20 }, { 20, 20, 40, 40 }, { 0, 0, 40, 40 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ 0, 0, 40, 40 }, { 20, 20, 40, 40 }, { 0, 0, 40, 40 }),
            std::make_tuple<Rect16, Rect16, Rect16>({ 10, 10, 30, 30 }, { 20, 20, 40, 40 }, { 10, 10, 40, 40 }));

        Rect16 res = l.Union(r);

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.TopLeft().x == expected.TopLeft().x);
        CHECK(res.TopLeft().y == expected.TopLeft().y);
        CHECK(res.BottomRight().x == expected.BottomRight().x);
        CHECK(res.BottomRight().y == expected.BottomRight().y);
    }

    SECTION("sequence") {
        using Sequence = std::array<Rect16, 8>;
        Sequence s;
        Rect16 l, expected;

        std::tie(l, s, expected) = GENERATE(
            std::make_tuple<Rect16, Sequence, Rect16>({ 0, 0, 10, 10 }, { {} }, { 0, 0, 10, 10 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ 0, 0, 20, 20 }, { { { 20, 20, 40, 40 } } }, { 0, 0, 40, 40 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ 0, 0, 20, 20 }, { { { 0, 20, 20, 40 }, { 20, 0, 40, 20 } } }, { 0, 0, 40, 40 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ 10, 10, 20, 20 }, { { { 0, 0, 10, 10 }, { 0, 20, 20, 40 }, { 20, 0, 40, 20 } } }, { 0, 0, 40, 40 }),
            std::make_tuple<Rect16, Sequence, Rect16>({ -20, -20, 0, 0 }, { { { 0, 0, 20, 20 } } }, { -20, -20, 20, 20 }));

        Rect16 res = l.Union(s);

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.TopLeft().x == expected.TopLeft().x);
        CHECK(res.TopLeft().y == expected.TopLeft().y);
        CHECK(res.BottomRight().x == expected.BottomRight().x);
        CHECK(res.BottomRight().y == expected.BottomRight().y);
    }
}

TEST_CASE("rectangle add padding", "[rectangle]") {
    Rect16 l, expected;
    padding_ui8_t p;

    std::tie(l, p, expected) = GENERATE(
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 20, 20 }, { 0, 0, 0, 0 }, { 0, 0, 20, 20 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 40, 40 }, { 20, 10, 30, 40 }, { -20, -10, 70, 80 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 20, 0, 40, 0 }, { -10, 10, 70, 30 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 20, 0, 0, 0 }, { -10, 10, 30, 30 }));

    l.AddPadding(p);

    CHECK(l.Width() == expected.Width());
    CHECK(l.Height() == expected.Height());
    CHECK(l.TopLeft().x == expected.TopLeft().x);
    CHECK(l.TopLeft().y == expected.TopLeft().y);
    CHECK(l.BottomRight().x == expected.BottomRight().x);
    CHECK(l.BottomRight().y == expected.BottomRight().y);
}

TEST_CASE("rectangle cut padding", "[rectangle]") {
    Rect16 l, expected;
    padding_ui8_t p;

    std::tie(l, p, expected) = GENERATE(
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 20, 20 }, { 0, 0, 0, 0 }, { 0, 0, 20, 20 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 0, 0, 40, 40 }, { 20, 10, 30, 40 }, { 0, 0, 0, 0 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 40, 30 }, { 10, 0, 10, 0 }, { 20, 10, 30, 30 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 10, 0, 10, 0 }, { 0, 0, 0, 0 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 30, 30 }, { 0, 10, 0, 10 }, { 0, 0, 0, 0 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 70, 70 }, { 10, 10, 20, 30 }, { 20, 20, 50, 40 }),
        std::make_tuple<Rect16, padding_ui8_t, Rect16>({ 10, 10, 40, 30 }, { 20, 0, 0, 0 }, { 30, 10, 40, 30 }));

    l.CutPadding(p);

    CHECK(l.Width() == expected.Width());
    CHECK(l.Height() == expected.Height());
    CHECK(l.TopLeft().x == expected.TopLeft().x);
    CHECK(l.TopLeft().y == expected.TopLeft().y);
    CHECK(l.BottomRight().x == expected.BottomRight().x);
    CHECK(l.BottomRight().y == expected.BottomRight().y);
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
                { 0, 0, 40, 40 }),
            std::make_tuple<Sequence, Rect16>({ { { 0, 0, 20, 20 },
                                                  { 0, 20, 20, 40 },
                                                  { 20, 0, 40, 20 } } },
                { 0, 0, 40, 40 }),
            std::make_tuple<Sequence, Rect16>({ { { 10, 10, 20, 20 },
                                                  { 0, 0, 10, 10 },
                                                  { 0, 20, 20, 40 },
                                                  { 20, 0, 40, 20 } } },
                { 0, 0, 40, 40 }),
            std::make_tuple<Sequence, Rect16>({ { { -20, -20, 0, 0 },
                                                  { 0, 0, 20, 20 } } },
                { -20, -20, 20, 20 }));

        Rect16 res = Rect16::Merge(s);

        CHECK(res.Width() == expected.Width());
        CHECK(res.Height() == expected.Height());
        CHECK(res.TopLeft().x == expected.TopLeft().x);
        CHECK(res.TopLeft().y == expected.TopLeft().y);
        CHECK(res.BottomRight().x == expected.BottomRight().x);
        CHECK(res.BottomRight().y == expected.BottomRight().y);
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

    SECTION("horizontal") {
        Sequence expected, result;
        Rect16 r;
        uint16_t span, count;

        std::tie(r, span, count, expected) = GENERATE(
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 0, 0 }, 10, 0, { { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 10, 4, { { { 0, 0, 10, 100 }, { 10, 0, 20, 100 }, { 20, 0, 30, 100 }, { 30, 0, 40, 100 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 30, 3, { { { 0, 0, 30, 100 }, { 30, 0, 60, 100 }, { 60, 0, 90, 100 } } }));

        size_t l = r.HorizontalSplit(result, span);
        CHECK(l == count);
        COMPARE_ARRAYS(expected, result);
    }

    SECTION("vertical") {
        Sequence expected, result;
        Rect16 r;
        uint16_t span, count;

        std::tie(r, span, count, expected) = GENERATE(
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 0, 0 }, 10, 0, { { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 10, 4, { { { 0, 0, 100, 10 }, { 0, 10, 100, 20 }, { 0, 20, 100, 30 }, { 0, 30, 100, 40 } } }),
            std::make_tuple<Rect16, uint16_t, uint16_t, Sequence>(
                { 0, 0, 100, 100 }, 30, 3, { { { 0, 0, 100, 30 }, { 0, 30, 100, 60 }, { 0, 60, 100, 90 } } })

        );

        size_t l = r.VerticalSplit(result, span);
        CHECK(l == count);
        COMPARE_ARRAYS(expected, result);
    }
}
