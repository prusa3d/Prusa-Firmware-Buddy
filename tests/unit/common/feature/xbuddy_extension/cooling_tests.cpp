#include <feature/xbuddy_extension/cooling.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Cooling PWM") {
    buddy::FanCooling cooling;

    SECTION("Manual, full pwm") {
        cooling.auto_control = false;
        cooling.target_pwm = 255;

        SECTION("With target temp") {
            cooling.target_temperature = 60;
        }

        SECTION("Without target temp") {}

        REQUIRE(cooling.compute_pwm(true, 54) == 255);
        REQUIRE(cooling.compute_pwm(false, 54) == 255);
    }

    SECTION("Manual, low PWM") {
        cooling.auto_control = false;
        cooling.target_pwm = 5;

        SECTION("Already running") {
            REQUIRE(cooling.compute_pwm(true, 54) == cooling.min_pwm);
        }

        SECTION("Initial kick") {
            REQUIRE(cooling.compute_pwm(false, 54) == 200);
        }
    }

    SECTION("Auto cooling, no target temp") {
        cooling.target_temperature = std::nullopt;

        REQUIRE(cooling.compute_pwm(true, 45) == 0);
        REQUIRE(cooling.target_pwm == 0);
    }

    SECTION("Auto cooling, cold chamber") {
        cooling.target_temperature = 60;

        REQUIRE(cooling.compute_pwm(false, 20) == 0);
        REQUIRE(cooling.target_pwm == 0);
    }

    SECTION("Auto cooling, slightly cool") {
        cooling.target_temperature = 60;

        SECTION("Not running, don't start") {
            REQUIRE(cooling.compute_pwm(false, 59) == 0);
            REQUIRE(cooling.target_pwm == 0);
        }

        SECTION("Already running, keep it up") {
            cooling.target_pwm = 70;
            REQUIRE(cooling.compute_pwm(true, 59) == cooling.min_pwm);
            REQUIRE(cooling.target_pwm == 1);
        }
    }

    SECTION("Auto cooling, exact temp") {
        cooling.target_temperature = 60;

        SECTION("Start") {
            REQUIRE(cooling.compute_pwm(false, 60) == 0);
            REQUIRE(cooling.target_pwm == 0);
        }

        SECTION("Already running") {
            REQUIRE(cooling.compute_pwm(true, 60) == cooling.min_pwm);
            REQUIRE(cooling.target_pwm == 1);
        }
    }

    SECTION("Auto cooling, really hot") {
        cooling.target_temperature = 60;

        REQUIRE(cooling.compute_pwm(true, 80) == 255);
        REQUIRE(cooling.target_pwm == 255);
    }

    SECTION("Auto cooling, slightly hot") {
        cooling.target_temperature = 60;

        SECTION("Already running") {
            REQUIRE(cooling.compute_pwm(true, 61) == cooling.min_pwm);
            REQUIRE(cooling.target_pwm == 25);
        }

        SECTION("Kick up") {
            REQUIRE(cooling.compute_pwm(false, 61) == 200);
            REQUIRE(cooling.target_pwm == 25);
        }
    }
}
