#include <feature/xbuddy_extension/cooling.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Cooling PWM") {
    buddy::FanCooling cooling;

    SECTION("Manual, full pwm") {
        cooling.auto_control = false;
        cooling.target_pwm = cooling.max_pwm;

        SECTION("With target temp") {
            cooling.target_temperature = 60;
        }

        SECTION("Without target temp") {}

        REQUIRE(cooling.compute_pwm(true, 54) == cooling.max_pwm);
        REQUIRE(cooling.compute_pwm(false, 54) == cooling.max_pwm);
    }

    SECTION("Manual, low PWM") {
        cooling.auto_control = false;
        cooling.target_pwm = 5;

        SECTION("Already running") {
            REQUIRE(cooling.compute_pwm(true, 54) == cooling.min_pwm);
        }

        SECTION("Initial kick") {
            REQUIRE(cooling.compute_pwm(false, 54) == cooling.spin_up_pwm);
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
        cooling.target_temperature = 30;

        REQUIRE(cooling.compute_pwm(true, 60) == cooling.soft_max_pwm);
        REQUIRE(cooling.target_pwm == cooling.soft_max_pwm);
    }

    SECTION("Auto cooling, slightly hot") {
        cooling.target_temperature = 60;

        SECTION("Already running") {
            REQUIRE(cooling.compute_pwm(true, 61) == cooling.min_pwm);
            REQUIRE(cooling.target_pwm == 10);
        }

        SECTION("Kick up") {
            REQUIRE(cooling.compute_pwm(false, 61) == cooling.spin_up_pwm);
            REQUIRE(cooling.target_pwm == 10);
        }
    }

    SECTION("Emergency cooling") {
        SECTION("Full power") {
            REQUIRE(cooling.compute_pwm(true, cooling.emergency_cooling_temp) == cooling.max_pwm);
        }

        SECTION("Half power") {
            REQUIRE(cooling.compute_pwm(true, cooling.emergency_cooling_temp - cooling.fans_max_temp_diff / 2) == cooling.max_pwm / 2);
        }

        SECTION("No emergency cooling") {
            REQUIRE(cooling.compute_pwm(true, cooling.emergency_cooling_temp - cooling.fans_max_temp_diff) == cooling.min_pwm);
            REQUIRE(cooling.compute_pwm(false, cooling.emergency_cooling_temp - cooling.fans_max_temp_diff) == 0);
        }

        REQUIRE(cooling.target_pwm == 0);
    }
}
