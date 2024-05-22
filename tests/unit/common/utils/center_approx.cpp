#include <array>
#include <span>
#include <catch2/catch.hpp>
#include <types.h>

#include <center_approx.hpp>

inline constexpr float ALLOWED_ERROR_MM { 0.01 };

static void test(const std::span<const xy_pos_t> points, const xy_pos_t true_center) {
    const xy_pos_t center = approximate_center(points);
    INFO("Center " << center.x << " " << center.y);
    CHECK((center - true_center).magnitude() < ALLOWED_ERROR_MM);
}

TEST_CASE("Fix circle 3 point") {
    test({ {
             { 184.938, 180 },
             { 178.163, 183.181 },
             { 178.163, 176.819 },
         } },
        { { 180.80372797047968, 180.0 } }); // from circle_fit import taubinSVD
}

TEST_CASE("Fit circle 12 point") {
    test({ {
             { 185.419, 180.2 },
             { 184.828, 182.428 },
             { 183.166, 183.991 },
             { 180.975, 184.438 },
             { 178.928, 183.747 },
             { 177.544, 182.181 },
             { 177.075, 180.2 },
             { 177.613, 178.262 },
             { 179.006, 176.781 },
             { 180.975, 176.125 },
             { 183.087, 176.538 },
             { 184.756, 178.019 },
         } },
        { { 181.25330479103351, 180.28446146100904 } }); // from circle_fit import taubinSVD
}
