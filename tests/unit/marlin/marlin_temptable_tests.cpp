#include <module/temperature/marlin_temptable.hpp>
#include <module/thermistor/thermistors.h>
#include <module/thermistor/thermistor_2005.h>
#include <module/thermistor/thermistor_1010.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("marlin_temptable::MarlinTemptableRawMinMax::2005") {
    const auto &tt = temptable_2005;
    REQUIRE(marlin_temptable_is_ntc(tt));
    CHECK(marlin_temptable_lookup(tt, OV(60)) == 320);
    CHECK(marlin_temptable_lookup(tt, OV(1023)) == -30);

    // { OV(530), 160 },
    // { OV(654), 140 },
    CHECK(marlin_temptable_lookup(tt, OV(500)) < 180);
    CHECK(marlin_temptable_lookup(tt, OV(500)) > 160);

    auto minmax = MarlinTemptableRawMinMax::compute(tt, 49, 141);

    // { OV(1003), 40 },
    CHECK(minmax.is_mintemp(OV(1003)));
    CHECK(!minmax.is_maxtemp(OV(1003)));

    // { OV(993), 50 },
    CHECK(!minmax.is_mintemp(OV(993)));
    CHECK(!minmax.is_maxtemp(OV(993)));

    // { OV(654), 140 },
    CHECK(!minmax.is_mintemp(OV(654)));
    CHECK(!minmax.is_maxtemp(OV(654)));

    // { OV(530), 160 },
    CHECK(!minmax.is_mintemp(OV(530)));
    CHECK(minmax.is_maxtemp(OV(530)));
}

TEST_CASE("marlin_temptable::MarlinTemptableRawMinMax::1010") {
    const auto &tt = temptable_1010;
    REQUIRE(!marlin_temptable_is_ntc(tt));
    CHECK(marlin_temptable_lookup(tt, OV(665)) == 225);
    CHECK(marlin_temptable_lookup(tt, OV(721)) == 375);

    // { OV( 626), 150 },
    // { OV( 640), 175 },
    CHECK(marlin_temptable_lookup(tt, OV(630)) > 150);
    CHECK(marlin_temptable_lookup(tt, OV(630)) < 175);

    auto minmax = MarlinTemptableRawMinMax::compute(tt, 51, 174);

    // { OV( 557),  50 },
    CHECK(minmax.is_mintemp(OV(557)));
    CHECK(!minmax.is_maxtemp(OV(557)));

    //  { OV( 577),  75 },
    CHECK(!minmax.is_mintemp(OV(577)));
    CHECK(!minmax.is_maxtemp(OV(577)));

    // { OV( 626), 150 },
    CHECK(!minmax.is_mintemp(OV(626)));
    CHECK(!minmax.is_maxtemp(OV(626)));

    // { OV( 640), 175 },
    CHECK(!minmax.is_mintemp(OV(640)));
    CHECK(minmax.is_maxtemp(OV(640)));
}

TEST_CASE("marlin_temptable::MarlinTemptableRawMinMax::1010 HT safety threshold") {
    // Regression: the HT hotend max nozzle temperature is 415°C; the table must extend above it so
    // compute() can pick a raw_max that actually corresponds to ~415°C. If the table
    // topped out at 400°C, raw_max collapsed to OV(729) and MAXTEMP fired at the user's
    // target (400°C), giving zero margin.
    const auto &tt = temptable_1010;
    auto minmax = MarlinTemptableRawMinMax::compute(tt, 5, 415);

    // 400°C must NOT trigger MAXTEMP (user-targetable max)
    CHECK(!minmax.is_maxtemp(OV(729)));

    // 425°C must trigger (above safety cutoff)
    CHECK(minmax.is_maxtemp(OV(736)));
}
