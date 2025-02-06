#include <feature/xbuddy_extension/cooling.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Cooling PWM") {
    buddy::FanCooling cooling;

    SECTION("Manual, full pwm") {
        cooling.set_auto_control(false);
        cooling.target_pwm = cooling.max_pwm;

        SECTION("With target temp") {
            cooling.target_temperature = 60;
        }

        SECTION("Without target temp") {}

        REQUIRE(cooling.compute_pwm_step(true, 54) == cooling.max_pwm);
        REQUIRE(cooling.compute_pwm_step(false, 54) == cooling.max_pwm);
    }

    SECTION("Manual, low PWM") {
        cooling.set_auto_control(false);
        cooling.target_pwm = 5;

        SECTION("Already running") {
            REQUIRE(cooling.compute_pwm_step(true, 54) == cooling.min_pwm);
        }

        SECTION("Initial kick") {
            REQUIRE(cooling.compute_pwm_step(false, 54) == cooling.spin_up_pwm);
        }
    }

    SECTION("Auto cooling, no target temp") {
        cooling.target_temperature = std::nullopt;

        REQUIRE(cooling.compute_pwm_step(true, 45) == 0);
        REQUIRE(cooling.target_pwm == 0);
    }

    SECTION("Auto cooling, cold chamber") {
        cooling.target_temperature = 60;

        REQUIRE(cooling.compute_pwm_step(false, 20) == 0);
        REQUIRE(cooling.target_pwm == 0);
    }

    SECTION("Auto cooling, slightly cool") {
        cooling.target_temperature = 60;

        SECTION("Not running, don't start") {
            REQUIRE(cooling.compute_pwm_step(false, 59) == 0);
            REQUIRE(cooling.target_pwm == 0);
        }
    }

    SECTION("Auto cooling, really hot") {
        cooling.target_temperature = 20;

        // it is not possible reach soft_max_pwm in one step within operational temperatures
        // so controll loop needs to be called several times before check
        uint8_t result = cooling.compute_pwm_step(true, 60);
        REQUIRE(result < cooling.get_soft_max_pwm());
        REQUIRE(result > 0);
        for (uint32_t i = 0; i < 10; i++) {
            cooling.compute_pwm_step(true, 60);
        }
        REQUIRE(cooling.compute_pwm_step(true, 60) == cooling.get_soft_max_pwm());
        REQUIRE(cooling.target_pwm == cooling.get_soft_max_pwm());
    }

    SECTION("Nonsense range test") {
        cooling.target_temperature = -100.0;
        REQUIRE(cooling.compute_pwm_step(true, 61) == cooling.get_soft_max_pwm());

        // due to previous regulation cycle, the target value must be multiplied
        cooling.target_temperature = 300.0;
        REQUIRE(cooling.compute_pwm_step(true, 61) == 0);
    }

    SECTION("Fan kick up speed") {
        // note: this test must follow a test, which has 0 as an output of compute_pwm_step
        cooling.target_temperature = 20;
        cooling.compute_pwm_step(true, 20); // set last_regulation_output to 0

        REQUIRE(cooling.compute_pwm_step(false, 0.5 + *cooling.target_temperature + cooling.min_pwm / cooling.proportional_constant) == cooling.spin_up_pwm);

        cooling.target_temperature = 300.0; // reset the regulation output to 0
        cooling.compute_pwm_step(true, 20);
        cooling.target_temperature = 20;
        cooling.compute_pwm_step(true, 20); // set last_regulation_output to 0

        REQUIRE(cooling.compute_pwm_step(true, 0.5 + *cooling.target_temperature + cooling.min_pwm / cooling.proportional_constant) == cooling.min_pwm);
    }

    SECTION("Overheating cooling") {
        SECTION("Full power") {
            cooling.target_temperature = 20;
            REQUIRE(cooling.compute_pwm_step(true, cooling.overheating_temp) == cooling.max_pwm);
            REQUIRE(cooling.get_overheating_temp_flag());
            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp + 1.0) == cooling.max_pwm);

            cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0); // one more regulation cycle is needed to recover
            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0) == cooling.get_soft_max_pwm());

            REQUIRE(cooling.compute_pwm_step(true, cooling.critical_temp) == cooling.max_pwm);
            REQUIRE(cooling.get_critical_temp_flag());

            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp + 1.0) == cooling.max_pwm);
            cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0); // one regulation cycle is needed to recover
            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0) == cooling.get_soft_max_pwm());
        }
    }
}
