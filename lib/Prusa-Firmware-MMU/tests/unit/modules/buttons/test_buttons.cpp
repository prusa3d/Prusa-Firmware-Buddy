#include "catch2/catch.hpp"
#include "../stubs/stub_adc.h"
#include "../stubs/stub_timebase.h"
#include "buttons.h"

static constexpr const uint16_t adcMaxValue = 1023U;

bool Step_Basic_One_Button_Test(mb::Buttons &b, uint8_t oversampleFactor, uint8_t testedButtonIndex, uint8_t otherButton1, uint8_t otherButton2) {
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // should detect the press but remain in detected state - wait for debounce
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(testedButtonIndex));
    CHECK(!b.ButtonPressed(otherButton1));
    CHECK(!b.ButtonPressed(otherButton2));

    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step();
        mt::IncMillis();
    }

    // just before the debounce trigger
    CHECK(!b.ButtonPressed(testedButtonIndex));
    CHECK(!b.ButtonPressed(otherButton1));
    CHECK(!b.ButtonPressed(otherButton2));

    // Tune the alg to overcome an edge case in debouncing timing - just in the unit test
    // This is very brittle, needs some work @TODO to clean up
    mt::IncMillis(4);
    b.Step(); // reset to waiting

    CHECK(b.ButtonPressed(testedButtonIndex));
    CHECK(!b.ButtonPressed(otherButton1));
    CHECK(!b.ButtonPressed(otherButton2));

    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // pressed again, still in debouncing state
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(testedButtonIndex));
    CHECK(!b.ButtonPressed(otherButton1));
    CHECK(!b.ButtonPressed(otherButton2));

    return true;
}

/// This test verifies the behaviour of a single button. The other buttons must remain intact.
bool Step_Basic_One_Button(hal::adc::TADCData &&d, uint8_t testedButtonIndex) {
    mt::ReinitTimebase();
    mb::Buttons b;

    // need to oversample the data as debouncing takes 100 cycles to accept a pressed button
    constexpr uint8_t oversampleFactor = config::buttonsDebounceMs;
    hal::adc::ReinitADC(config::buttonsADCIndex, std::move(d), oversampleFactor);

    uint8_t otherButton1 = 1, otherButton2 = 2;
    switch (testedButtonIndex) {
    case 1:
        otherButton1 = 0;
        break;
    case 2:
        otherButton2 = 0;
        break;
    default:
        break; // no change
    }

    return Step_Basic_One_Button_Test(b, oversampleFactor, testedButtonIndex, otherButton1, otherButton2);
}

constexpr uint16_t M1(uint16_t v) {
    return static_cast<uint16_t>(v - 1U);
}
TEST_CASE("buttons::Step-basic-button", "[buttons]") {
    for (uint8_t i = 0; i < config::buttonCount; ++i) {
        CHECK(Step_Basic_One_Button({ M1(config::buttonADCLimits[i][0]),
                                        config::buttonADCLimits[i][1],
                                        adcMaxValue },
            i));
    }
}

/// This test has to verify the independency of buttons - the ADC reads one button after the other
/// and the Buttons class should press first button and release, then the second one and then the third one
/// without being reinitialized.
TEST_CASE("buttons::Step-basic-button-one-after-other", "[buttons]") {
    hal::adc::TADCData d({ M1(config::buttonADCLimits[0][0]), config::buttonADCLimits[0][0] + 1, adcMaxValue,
        M1(config::buttonADCLimits[1][0]), config::buttonADCLimits[1][0] + 1, adcMaxValue,
        M1(config::buttonADCLimits[2][0]), config::buttonADCLimits[2][0] + 1, adcMaxValue });
    mb::Buttons b;

    // need to oversample the data as debouncing takes 100 cycles to accept a pressed button
    constexpr uint8_t oversampleFactor = config::buttonsDebounceMs;
    hal::adc::ReinitADC(config::buttonsADCIndex, std::move(d), oversampleFactor);

    CHECK(Step_Basic_One_Button_Test(b, oversampleFactor, 0, 1, 2));
    CHECK(Step_Basic_One_Button_Test(b, oversampleFactor, 1, 0, 2));
    CHECK(Step_Basic_One_Button_Test(b, oversampleFactor, 2, 0, 1));
}

/// This test tries to simulate a bouncing effect on data from ADC on the first button
TEST_CASE("buttons::Step-debounce-one-button", "[buttons]") {
    // make a bounce event on the first press
    hal::adc::TADCData d({ 5, adcMaxValue, 5, 9, 6, 7, 8, adcMaxValue, adcMaxValue });

    // need to oversample the data as debouncing takes 100 cycles to accept a pressed button
    constexpr uint8_t oversampleFactor = 25;
    hal::adc::ReinitADC(config::buttonsADCIndex, std::move(d), oversampleFactor);
    mt::ReinitTimebase();

    mb::Buttons b;

    // 5
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // should detect the press but remain in detected state - wait for debounce
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));

    // 1023
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // reset to waiting
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));

    // 5
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // pressed again, still in debouncing state
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));

    // 9
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // no change
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Left));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Right));

    // 6
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // no change
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));

    // 7
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // one step from "pressed"
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));

    // 8
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // fifth set of samples - should report "pressed" finally
        mt::IncMillis();
    }
    CHECK(b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));

    // 1023
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // sixth set of samples - button released (no debouncing on release)
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));

    // 1023
    for (uint8_t i = 0; i < oversampleFactor; ++i) {
        b.Step(); // seventh set of samples - still released
        mt::IncMillis();
    }
    CHECK(!b.ButtonPressed(mb::Right));
    CHECK(!b.ButtonPressed(mb::Middle));
    CHECK(!b.ButtonPressed(mb::Left));
}
