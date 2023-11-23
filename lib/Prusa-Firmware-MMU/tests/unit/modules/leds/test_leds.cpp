#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "leds.h"
#include "shr16.h"
#include "../stubs/stub_timebase.h"
#include "../../../../src/modules/timebase.h"

uint16_t millis = 0;

namespace hal {
namespace shr16 {
extern uint16_t shr16_v_copy;
} // namespace shr16
} // namespace hal

/// LEDS - hardcoded
#define SHR16_LEDR0 0x0080
#define SHR16_LEDG0 0x0040
#define SHR16_LEDR1 0x8000
#define SHR16_LEDG1 0x4000
#define SHR16_LEDR2 0x2000
#define SHR16_LEDG2 0x1000
#define SHR16_LEDR3 0x0800
#define SHR16_LEDG3 0x0400
#define SHR16_LEDR4 0x0200
#define SHR16_LEDG4 0x0100

TEST_CASE("leds::single", "[leds]") {
    using namespace hal::shr16;
    mt::ReinitTimebase(); // reset timing

    ml::LEDs leds;

    uint8_t index;
    ml::Color color;
    uint16_t shr16_register;
    std::tie(index, color, shr16_register) = GENERATE(
        std::make_tuple(0, ml::green, SHR16_LEDG0),
        std::make_tuple(0, ml::red, SHR16_LEDR0),
        std::make_tuple(1, ml::green, SHR16_LEDG1),
        std::make_tuple(1, ml::red, SHR16_LEDR1),
        std::make_tuple(2, ml::green, SHR16_LEDG2),
        std::make_tuple(2, ml::red, SHR16_LEDR2),
        std::make_tuple(3, ml::green, SHR16_LEDG3),
        std::make_tuple(3, ml::red, SHR16_LEDR3),
        std::make_tuple(4, ml::green, SHR16_LEDG4),
        std::make_tuple(4, ml::red, SHR16_LEDR4));

    shr16.Init(); // clears the register for the test

    // turn LED on
    leds.SetMode(index, color, ml::on);
    leds.Step();
    mt::IncMillis();
    CHECK(leds.LedOn(index, color) == true);
    CHECK(shr16_v_copy == shr16_register);

    // turn LED off
    leds.SetMode(index, color, ml::off);
    leds.Step();
    mt::IncMillis();
    CHECK(leds.LedOn(index, color) == false);
    CHECK(shr16_v_copy == 0);
}

void TestBlink(uint8_t index, ml::Color color, uint16_t shr16_register, bool shouldBeOn, ml::Mode blinkMode) {
    using namespace hal::shr16;
    mt::ReinitTimebase(); // reset timing
    ml::LEDs leds;

    leds.SetMode(index, color, blinkMode);
    leds.Step();
    mt::IncMillis();

    REQUIRE(leds.LedOn(index, color) == shouldBeOn);
    CHECK(shr16_v_copy == (shouldBeOn ? shr16_register : 0));

    // test 4 seconds of blinking
    for (uint8_t s = 1; s < 4; ++s) {
        // one second elapsed ;)
        mt::IncMillis(config::ledBlinkPeriodMs / 2);
        leds.Step();
        shouldBeOn = !shouldBeOn;
        CHECK(leds.LedOn(index, color) == shouldBeOn);
        CHECK(shr16_v_copy == (shouldBeOn ? shr16_register : 0));
    }

    // turn LED off
    leds.SetMode(index, color, ml::off);
    leds.Step();
    mt::IncMillis();
    CHECK(leds.LedOn(index, color) == false);
    CHECK(shr16_v_copy == 0);
}

TEST_CASE("leds::blink0-single", "[leds]") {
    using namespace hal::shr16;
    uint8_t index;
    ml::Color color;
    uint16_t shr16_register;
    std::tie(index, color, shr16_register) = GENERATE(
        std::make_tuple(0, ml::green, SHR16_LEDG0),
        std::make_tuple(0, ml::red, SHR16_LEDR0),
        std::make_tuple(1, ml::green, SHR16_LEDG1),
        std::make_tuple(1, ml::red, SHR16_LEDR1),
        std::make_tuple(2, ml::green, SHR16_LEDG2),
        std::make_tuple(2, ml::red, SHR16_LEDR2),
        std::make_tuple(3, ml::green, SHR16_LEDG3),
        std::make_tuple(3, ml::red, SHR16_LEDR3),
        std::make_tuple(4, ml::green, SHR16_LEDG4),
        std::make_tuple(4, ml::red, SHR16_LEDR4));

    shr16.Init(); // clears the register for the test

    // set LED into blink0 mode - on in even periods of 1s intervals, starts as OFF
    TestBlink(index, color, shr16_register, false, ml::blink0);

    TestBlink(index, color, shr16_register, true, ml::blink1);
}

void TestBlinkOverflow(uint16_t period) {
    using namespace hal::shr16;

    // reset timing to a stage close to millis overflow
    mt::ReinitTimebase(0x10000U - period / 2 + 1);

    ml::LEDs leds;
    uint8_t index = 0;
    ml::Color color = ml::green;
    uint16_t ms = mt::timebase.Millis();
    bool shouldBeOn = ((ms / (period / 2)) & 0x01U) != 0;
    uint16_t shr16_register = SHR16_LEDG0;

    leds.SetMode(index, color, ml::blink0);
    leds.Step();

    REQUIRE(leds.LedOn(index, color) == shouldBeOn);
    CHECK(shr16_v_copy == (shouldBeOn ? shr16_register : 0));

    // advance timebase with overflow
    mt::IncMillis(period / 2);

    // the LED must have changed its state
    leds.Step();

    REQUIRE(leds.LedOn(index, color) != shouldBeOn);
    CHECK(shr16_v_copy == (shouldBeOn ? 0 : shr16_register));
}

TEST_CASE("leds::blink0-overflow", "[leds]") {
    // this is a check for a correct FW configuration
    TestBlinkOverflow(config::ledBlinkPeriodMs);
}
