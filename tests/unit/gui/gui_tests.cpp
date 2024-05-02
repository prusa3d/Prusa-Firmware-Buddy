#include <catch2/catch.hpp>
#include <numeric_input_config.hpp>

TEST_CASE("gui::numeric_input_config") {
    SECTION("max_value_strlen") {
        CHECK(NumericInputConfig::num_digits(0) == 1);
        CHECK(NumericInputConfig::num_digits(1) == 1);
        CHECK(NumericInputConfig::num_digits(9) == 1);
        CHECK(NumericInputConfig::num_digits(10) == 2);
        CHECK(NumericInputConfig::num_digits(99) == 2);
        CHECK(NumericInputConfig::num_digits(100) == 3);
        CHECK(NumericInputConfig::num_digits(999) == 3);
        CHECK(NumericInputConfig::num_digits(1000) == 4);
        CHECK(NumericInputConfig::num_digits(9999) == 4);
        CHECK(NumericInputConfig::num_digits(10000) == 5);
        CHECK(NumericInputConfig::num_digits(99999) == 5);

        CHECK(NumericInputConfig { .min_value = 0, .max_value = 0 }.max_value_strlen() == 1);
        CHECK(NumericInputConfig { .min_value = 0, .max_value = 9 }.max_value_strlen() == 1);
        CHECK(NumericInputConfig { .min_value = -1, .max_value = 9 }.max_value_strlen() == 2); // - sign takes one space
        CHECK(NumericInputConfig { .min_value = 0, .max_value = 10 }.max_value_strlen() == 2);
        CHECK(NumericInputConfig { .min_value = 0, .max_value = 99 }.max_value_strlen() == 2);
        CHECK(NumericInputConfig { .min_value = -9, .max_value = 10 }.max_value_strlen() == 2);
        CHECK(NumericInputConfig { .min_value = -99, .max_value = 10 }.max_value_strlen() == 3);
        CHECK(NumericInputConfig { .min_value = 0, .max_value = 999 }.max_value_strlen() == 3);
        CHECK(NumericInputConfig { .min_value = 0, .max_value = 1000 }.max_value_strlen() == 4);
        CHECK(NumericInputConfig { .min_value = 0.01, .max_value = 0.05, .max_decimal_places = 0 }.max_value_strlen() == 1);
        CHECK(NumericInputConfig { .min_value = 0, .max_value = 1, .max_decimal_places = 2 }.max_value_strlen() == 4); // 0.01
        CHECK(NumericInputConfig { .min_value = -0.01, .max_value = 1, .max_decimal_places = 2 }.max_value_strlen() == 5); // -0.01
    }
}
