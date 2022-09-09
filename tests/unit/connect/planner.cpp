#include "time_mock.h"
#include "mock_printer.h"

#include <planner.hpp>

#include <catch2/catch.hpp>

using namespace connect_client;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::move;

namespace {

struct Test {
    Printer::Params params;
    MockPrinter printer;
    Planner planner;

    Test(ActionResult init_result = ActionResult::Ok)
        : params(params_idle())
        , printer(this->params)
        , planner(printer) {
        // Intro exchange
        event_info();
        planner.action_done(init_result);
    }

    void event_type(EventType event_type) {
        const auto action = planner.next_action();
        const auto *event = get_if<Event>(&action);
        REQUIRE(event != nullptr);
        REQUIRE(event->type == event_type);
    }

    void event_info() {
        event_type(EventType::Info);
    }

    Duration consume_sleep() {
        const auto sleep = planner.next_action();
        const auto sleep_typed = get_if<Sleep>(&sleep);
        REQUIRE(sleep_typed != nullptr);
        // Pretend to sleep a bit by moving the clock.
        advance_time(sleep_typed->milliseconds);

        return sleep_typed->milliseconds;
    }

    void consume_telemetry() {
        Action action = planner.next_action();
        REQUIRE(holds_alternative<SendTelemetry>(action));
        planner.action_done(ActionResult::Ok);
    }
};

}

TEST_CASE("Success scenario") {
    Test test;

    test.consume_telemetry();

    test.consume_sleep();

    // But now, after we have slept, we shall send some more telemetries
    test.consume_telemetry();
}

TEST_CASE("Retries early") {
    Test test(ActionResult::Failed);

    // It'll provide a small sleep before trying again.
    Duration sleep1 = test.consume_sleep();

    // Now it'll retry the event.
    test.event_info();
    test.planner.action_done(ActionResult::Failed);

    // If it fails again, it schedules a longer sleep.
    Duration sleep2 = test.consume_sleep();
    REQUIRE(sleep1 < sleep2);

    // If we succeed after few retries, we move on to the telemetry
    test.event_info();
    test.planner.action_done(ActionResult::Ok);

    // Back to telemetries after the action
    test.consume_telemetry();
}

TEST_CASE("Reinit after several failures") {
    Test test;

    // Eventually, it stops trying to send the telemetry and goes back to trying to send Info
    Action action;
    do {
        action = test.planner.next_action();
        test.planner.action_done(ActionResult::Failed);
        test.consume_sleep();
    } while (holds_alternative<SendTelemetry>(action));

    REQUIRE(holds_alternative<Event>(action));
    REQUIRE(get<Event>(action).type == EventType::Info);

    // We'll succeed eventually
    test.event_info();
    test.planner.action_done(ActionResult::Ok);

    // Goes back to telemetry after sucessful reinit
    test.consume_telemetry();
}

TEST_CASE("Unknown / broken command refused") {
    Test test;

    // Commands come as replies to telemetries
    test.consume_telemetry();

    SECTION("Unknown") {
        test.planner.command(Command {
            1,
            UnknownCommand {},
        });
    }

    SECTION("Broken") {
        test.planner.command(Command {
            1,
            BrokenCommand {},
        });
    }

    test.event_type(EventType::Rejected);
    test.planner.action_done(ActionResult::Ok);

    test.consume_telemetry();
}

TEST_CASE("Send info request") {
    Test test;

    // Commands come as replies to telemetries
    test.consume_telemetry();

    test.planner.command(Command {
        1,
        SendInfo {},
    });

    test.event_info();
}

TEST_CASE("Submit gcode") {
    Test test;

    // Commands come as replies to telemetries
    test.consume_telemetry();

    SharedBuffer buffer;
    auto borrow = buffer.borrow().value();
    auto command = Command::gcode_command(1, "M100\n  M200 X10 Y20 \r\n\r\n \nM300", move(borrow));

    test.planner.command(command);

    // Not processed yet, it processes in the background.
    REQUIRE(test.printer.submitted_gcodes.size() == 0);
    // First, it accepts the command.
    test.event_type(EventType::Accepted);
    test.planner.action_done(ActionResult::Ok);

    // We always send a telemetry after an event.
    test.consume_telemetry();

    // (Still no background processing with the Accepted, that happens right away).
    REQUIRE(test.printer.submitted_gcodes.size() == 0);

    // More processing will cause Finished, because the background processing
    // is "fast", no blocking in here.
    test.event_type(EventType::Finished);
    REQUIRE(test.printer.submitted_gcodes.size() == 3);

    REQUIRE(test.printer.submitted_gcodes[0] == "M100");
    REQUIRE(test.printer.submitted_gcodes[1] == "M200 X10 Y20");
    REQUIRE(test.printer.submitted_gcodes[2] == "M300");
}

// TODO: Tests for unknown commands and such
