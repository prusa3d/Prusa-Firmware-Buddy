#include <printer.hpp>

#include "mock_printer.h"

#include <catch2/catch_test_macros.hpp>

using std::nullopt;
using namespace connect_client;

TEST_CASE("connect::Params CRC", "[connect]") {
    Printer::Params params(nullopt);
    uint32_t base_crc = params.telemetry_fingerprint(true);

    SECTION("Nozzle & bed temperature") {
        params.temp_bed = 50;
        params.physical_tools[PhysicalToolIndex::from_raw(0)].temp_nozzle = 150;
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

TEST_CASE("connect::INFO fingerprint covers the reported material", "[connect]") {
    auto params = params_idle();
    MockPrinter printer(params);
    const uint32_t without_material = printer.info_fingerprint();

    printer.loaded_material = FilamentTypeParameters {
        .name = "PLA",
        .nozzle_temperature = 215,
        .nozzle_preheat_temperature = 170,
        .heatbed_temperature = 60,
    };
    const uint32_t with_material = printer.info_fingerprint();
    REQUIRE(with_material != without_material);

    SECTION("Rendered fields") {
        SECTION("name") {
            printer.loaded_material->name = "PETG";
        }
        SECTION("color") {
            printer.loaded_material->color = Color::from_rgb(0x12, 0xab, 0xef);
        }
        SECTION("nozzle temperature") {
            printer.loaded_material->nozzle_temperature = 230;
        }
        SECTION("preheat temperature") {
            printer.loaded_material->nozzle_preheat_temperature = 180;
        }
        SECTION("bed temperature") {
            printer.loaded_material->heatbed_temperature = 90;
        }
        SECTION("abrasive") {
            printer.loaded_material->is_abrasive = true;
        }
        REQUIRE(printer.info_fingerprint() != with_material);
    }

    SECTION("A disabled virtual tool renders no material") {
        params.virtual_tool_mask = 0;
        const uint32_t disabled = printer.info_fingerprint();
        printer.loaded_material.reset();
        REQUIRE(printer.info_fingerprint() == disabled);
    }
}
