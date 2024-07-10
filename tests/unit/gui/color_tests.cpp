#include <catch2/catch.hpp>
#include <utils/color.hpp>

TEST_CASE("color::basic_tests") {
    CHECK(Color::from_raw(0).raw == 0);
    CHECK(Color::from_raw(12345).raw == 12345);
    CHECK(Color::from_raw(0xAABBCC).r == 0xaa);
    CHECK(Color::from_raw(0xAABBCC).g == 0xbb);
    CHECK(Color::from_raw(0xAABBCC).b == 0xcc);

    CHECK(Color::from_string("#11AA22")->raw == 0x11aa22);
    CHECK(Color::from_string("WHITE")->raw == 0xffffff);
    CHECK(Color::from_string("12345")->raw == 12345);

    CHECK(!Color::from_string("BLBOST").has_value());
    CHECK(!Color::from_string("#NIC").has_value());
}
