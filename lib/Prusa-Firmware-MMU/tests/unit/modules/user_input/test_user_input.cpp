#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "../stubs/stub_adc.h"
#include "../stubs/stub_timebase.h"
#include "buttons.h"
#include "../hal/adc.h"
#include "user_input.h"

CATCH_REGISTER_ENUM(mui::Event,
    mui::Event::NoEvent,
    mui::Event::Left,
    mui::Event::Middle,
    mui::Event::Right,
    mui::Event::FromPrinter)

void PressButtonAndDebounce(uint8_t btnIndex) {
    hal::adc::SetADC(config::buttonsADCIndex, config::buttonADCLimits[btnIndex][0] + 1);
    while (!mb::buttons.ButtonPressed(btnIndex)) {
        mb::buttons.Step();
        mui::userInput.Step();
        mt::IncMillis();
    }
}

void UnpressButtons() {
    hal::adc::SetADC(config::buttonsADCIndex, config::buttonADCMaxValue);
    mb::buttons.Step(); // should fall into "waiting" state
    REQUIRE(mb::buttons.AnyButtonPressed() == false);
}

TEST_CASE("user_input::printer_in_charge", "[user_input]") {
    uint8_t button;
    mui::Event event;
    std::tie(button, event) = GENERATE(
        std::make_tuple(mb::Left, mui::Event::Left),
        std::make_tuple(mb::Middle, mui::Event::Middle),
        std::make_tuple(mb::Right, mui::Event::Right));

    mt::ReinitTimebase();
    mb::Buttons b;
    // reset UI
    new (&mui::userInput) mui::UserInput();
    REQUIRE_FALSE(mui::userInput.PrinterInCharge());

    // set printer in charge
    mui::userInput.SetPrinterInCharge(true);
    REQUIRE(mui::userInput.PrinterInCharge());

    // put some button into the buffer - should be marked as "from printer"
    mui::userInput.ProcessMessage(button);
    // i.e. we should NOT be able to extract that message with ConsumeEventForPrinter()
    REQUIRE(mui::userInput.ConsumeEventForPrinter() == mui::NoEvent);
    REQUIRE(mui::userInput.AnyEvent());
    // but we should be able to extract that message with ConsumeEvent()
    REQUIRE(mui::userInput.ConsumeEvent() == event);
    REQUIRE_FALSE(mui::userInput.AnyEvent());

    // press a button on the MMU
    PressButtonAndDebounce(button);
    REQUIRE(mb::buttons.ButtonPressed(button));
    // we should NOT be able to extract the event with ConsumeEvent
    REQUIRE(mui::userInput.ConsumeEvent() == mui::NoEvent);
    REQUIRE(mui::userInput.AnyEvent());
    // but we should be able to extract that message with ConsumeEventForPrinter
    REQUIRE(mui::userInput.ConsumeEventForPrinter() == event);
    REQUIRE_FALSE(mui::userInput.AnyEvent());
}

template <typename T>
bool OtherButtons(uint8_t activeButton, T f) {
    for (uint8_t button = 0; button < config::buttonCount; ++button) {
        if (button != activeButton) {
            if (!f(button))
                return false;
        }
    }
    return true;
}

TEST_CASE("user_input::button_pressed_MMU", "[user_input]") {
    uint8_t button;
    mui::Event event;
    std::tie(button, event) = GENERATE(
        std::make_tuple(mb::Left, mui::Event::Left),
        std::make_tuple(mb::Middle, mui::Event::Middle),
        std::make_tuple(mb::Right, mui::Event::Right));

    // create a button press
    mt::ReinitTimebase();
    mb::Buttons b;

    // reset UI
    new (&mui::userInput) mui::UserInput();
    REQUIRE_FALSE(mui::userInput.PrinterInCharge());
    REQUIRE(mui::userInput.LastButtonState(button) == false);

    PressButtonAndDebounce(button);
    REQUIRE(mb::buttons.ButtonPressed(button));
    REQUIRE(OtherButtons(button, [&](uint8_t b) { return mb::buttons.ButtonPressed(b) == false; }));

    // check internal flags
    REQUIRE(mui::userInput.LastButtonState(button) == true);
    REQUIRE(OtherButtons(button, [&](uint8_t b) { return mui::userInput.LastButtonState(b) == false; }));

    REQUIRE(mui::userInput.eventQueue.count() == 1);

    // step userInput again (prevent multiple generated events when button still pressed)
    for (uint8_t i = 0; i < 10; ++i) {
        mui::userInput.Step();
        REQUIRE(mui::userInput.LastButtonState(button) == true);
        REQUIRE(OtherButtons(button, [&](uint8_t b) { return mui::userInput.LastButtonState(b) == false; }));
        REQUIRE(mui::userInput.eventQueue.count() == 1);
    }

    // release the button
    UnpressButtons();
    mui::userInput.Step();
    REQUIRE(mui::userInput.LastButtonState(button) == false);
    REQUIRE(OtherButtons(button, [&](uint8_t b) { return mui::userInput.LastButtonState(b) == false; }));
    REQUIRE(mui::userInput.eventQueue.count() == 1);

    // we should be able to extract the event with ConsumeEvent
    REQUIRE(mui::userInput.AnyEvent());
    REQUIRE(mui::userInput.ConsumeEvent() == event);
    REQUIRE_FALSE(mui::userInput.AnyEvent());
}

TEST_CASE("user_input::empty_queue", "[user_input]") {
    mt::ReinitTimebase();
    mb::Buttons b;

    // reset UI
    new (&mui::userInput) mui::UserInput();
    REQUIRE_FALSE(mui::userInput.PrinterInCharge());

    REQUIRE_FALSE(mui::userInput.AnyEvent());
    // try extracting something
    REQUIRE(mui::userInput.ConsumeEvent() == mui::NoEvent);
    REQUIRE(mui::userInput.ConsumeEventForPrinter() == mui::NoEvent);
}

TEST_CASE("user_input::lastStates", "[user_input]") {
    uint8_t button;
    uint8_t bitMask;
    std::tie(button, bitMask) = GENERATE(
        std::make_tuple(mb::Left, 4),
        std::make_tuple(mb::Middle, 2),
        std::make_tuple(mb::Right, 1));

    new (&mui::userInput) mui::UserInput();

    mui::userInput.FlipLastButtonState(button);
    REQUIRE(mui::userInput.lastButtonStates == bitMask);

    mui::userInput.FlipLastButtonState(button);
    REQUIRE(mui::userInput.lastButtonStates == 0);
}
