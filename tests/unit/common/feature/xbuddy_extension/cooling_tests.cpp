#include <feature/xbuddy_extension/cooling.hpp>

#include <catch2/catch.hpp>

using namespace buddy;

TEST_CASE("Cooling PWM") {
    buddy::FanCooling cooling;

    SECTION("Manual, full pwm") {
        std::optional<Temperature> target_temperature;
        SECTION("With target temp") {
            target_temperature = 60;
        }

        SECTION("Without target temp") {}

        REQUIRE(cooling.compute_pwm_step(true, 54, target_temperature, cooling.max_pwm) == cooling.max_pwm);
        REQUIRE(cooling.compute_pwm_step(false, 54, target_temperature, cooling.max_pwm) == cooling.max_pwm);
    }

    SECTION("Manual, low PWM") {
        const PWM255 target_pwm { 5 };

        SECTION("Already running") {
            REQUIRE(cooling.compute_pwm_step(true, 54, std::nullopt, target_pwm) == cooling.min_pwm);
        }

        SECTION("Initial kick") {
            REQUIRE(cooling.compute_pwm_step(false, 54, std::nullopt, target_pwm) == cooling.spin_up_pwm);
        }
    }

    SECTION("Auto cooling, no target temp") {
        REQUIRE(cooling.compute_pwm_step(true, 45, std::nullopt, pwm_auto).value == 0);
    }

    SECTION("Auto cooling, cold chamber") {
        REQUIRE(cooling.compute_pwm_step(false, 20, 60, pwm_auto).value == 0);
    }

    SECTION("Auto cooling, slightly cool") {
        SECTION("Not running, don't start") {
            REQUIRE(cooling.compute_pwm_step(false, 59, 60, pwm_auto).value == 0);
        }
    }

    SECTION("Auto cooling, really hot") {
        const std::optional<Temperature> target_temperature = 20;

        // it is not possible reach soft_max_pwm in one step within operational temperatures
        // so controll loop needs to be called several times before check
        const auto result = cooling.compute_pwm_step(true, 60, target_temperature, pwm_auto);
        REQUIRE(result < cooling.get_soft_max_pwm());
        REQUIRE(result > PWM255 { 0 });
        for (uint32_t i = 0; i < 10; i++) {
            cooling.compute_pwm_step(true, 60, target_temperature, pwm_auto);
        }
        REQUIRE(cooling.compute_pwm_step(true, 60, target_temperature, pwm_auto) == cooling.get_soft_max_pwm());
    }

    SECTION("Nonsense range test") {
        REQUIRE(cooling.compute_pwm_step(true, 61, -100, pwm_auto) == cooling.get_soft_max_pwm());

        // due to previous regulation cycle, the target value must be multiplied
        REQUIRE(cooling.compute_pwm_step(true, 61, 300, pwm_auto) == PWM255 { 0 });
    }

    SECTION("Fan kick up speed") {
        // note: this test must follow a test, which has 0 as an output of compute_pwm_step
        Temperature target_temperature = 20;
        cooling.compute_pwm_step(true, 20, 20, pwm_auto); // set last_regulation_output to 0

        REQUIRE(cooling.compute_pwm_step(false, 0.5 + target_temperature + cooling.min_pwm.value / cooling.proportional_constant, target_temperature, pwm_auto) == cooling.spin_up_pwm);

        target_temperature = 300.0; // reset the regulation output to 0
        cooling.compute_pwm_step(true, 20, target_temperature, pwm_auto);
        target_temperature = 20;
        cooling.compute_pwm_step(true, 20, target_temperature, pwm_auto); // set last_regulation_output to 0

        REQUIRE(cooling.compute_pwm_step(true, 0.5 + target_temperature + cooling.min_pwm.value / cooling.proportional_constant, target_temperature, pwm_auto) == cooling.min_pwm);
    }

    SECTION("Overheating cooling") {
        SECTION("Full power") {
            const Temperature target_temperature = 20;
            REQUIRE(cooling.compute_pwm_step(true, cooling.overheating_temp, target_temperature, pwm_auto) == cooling.max_pwm);
            REQUIRE(cooling.get_overheating_temp_flag());
            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp + 1.0, target_temperature, pwm_auto) == cooling.max_pwm);

            cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0, target_temperature, pwm_auto); // one more regulation cycle is needed to recover
            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0, target_temperature, pwm_auto) == cooling.get_soft_max_pwm());

            REQUIRE(cooling.compute_pwm_step(true, cooling.critical_temp, target_temperature, pwm_auto) == cooling.max_pwm);
            REQUIRE(cooling.get_critical_temp_flag());

            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp + 1.0, target_temperature, pwm_auto) == cooling.max_pwm);
            cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0, target_temperature, pwm_auto); // one regulation cycle is needed to recover
            REQUIRE(cooling.compute_pwm_step(true, cooling.recovery_temp - 1.0, target_temperature, pwm_auto) == cooling.get_soft_max_pwm());
        }
    }
}
