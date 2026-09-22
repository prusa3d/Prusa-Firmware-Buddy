#include <printer.hpp>

#include <catch2/catch_test_macros.hpp>

using std::nullopt;
using namespace connect_client;

TEST_CASE("Params CRC") {
    Printer::Params params(nullopt);
    uint32_t base_crc = params.telemetry_fingerprint(true);

    SECTION("Nozzle & bed temperature") {
        params.temp_bed = 50;
        params.slots[0].temp_nozzle = 150;
    }

    SECTION("target_bed") {
        params.target_bed = 60;
    }

    SECTION("chamber target_temp") {
        params.chamber_info.target_temp = 40;
    }

    SECTION("chamber fan_pwm_target") {
        params.chamber_info.fan_pwm_target = 50;
    }

    SECTION("chamber led_intensity") {
        params.chamber_info.led_intensity = 100;
    }

    REQUIRE(base_crc != params.telemetry_fingerprint(true));
}
