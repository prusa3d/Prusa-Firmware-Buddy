#include <printer.hpp>

#include <catch2/catch.hpp>

using namespace connect_client;

TEST_CASE("Params CRC") {
    Printer::Params params;

    uint32_t empty_crc = params.telemetry_fingerprint(true);

    params.temp_bed = 50;
    params.temp_nozzle = 150;

    uint32_t warm_crc = params.telemetry_fingerprint(true);

    REQUIRE(empty_crc != warm_crc);
}
