#include "time_mock.h"

#include <planner.hpp>

#include <catch2/catch.hpp>

using namespace con;
using std::get;
using std::get_if;
using std::holds_alternative;

namespace {

Duration consume_sleep(Planner &planner) {
    const auto sleep = planner.next_action();
    const auto sleep_typed = get_if<Sleep>(&sleep);
    REQUIRE(sleep_typed != nullptr);
    // Pretend to sleep a bit by moving the clock.
    advance_time(sleep_typed->milliseconds);

    return sleep_typed->milliseconds;
}

void event_type(Planner &planner, EventType event_type) {
    const auto action = planner.next_action();
    const auto *event = get_if<Event>(&action);
    REQUIRE(event != nullptr);
    REQUIRE(event->type == event_type);
}

void event_info(Planner &planner) {
    event_type(planner, EventType::Info);
}

}

TEST_CASE("Success scenario") {
    Planner planner;

    event_info(planner);
    planner.action_done(ActionResult::Ok);

    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
    planner.action_done(ActionResult::Ok);

    consume_sleep(planner);

    // But now, after we have slept, we shall send some more telemetries
    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
}

TEST_CASE("Retries early") {
    Planner planner;

    event_info(planner);
    planner.action_done(ActionResult::Failed);

    // It'll provide a small sleep before trying again.
    Duration sleep1 = consume_sleep(planner);

    // Now it'll retry the event.
    event_info(planner);
    planner.action_done(ActionResult::Failed);

    // If it fails again, it schedules a longer sleep.
    Duration sleep2 = consume_sleep(planner);
    REQUIRE(sleep1 < sleep2);

    // If we succeed after few retries, we move on to the telemetry
    event_info(planner);
    planner.action_done(ActionResult::Ok);
    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
}

TEST_CASE("Reinit after several failures") {
    Planner planner;

    event_info(planner);
    planner.action_done(ActionResult::Ok);

    // Eventually, it stops trying to send the telemetry and goes back to trying to send Info
    Action action;
    do {
        action = planner.next_action();
        planner.action_done(ActionResult::Failed);
        consume_sleep(planner);
    } while (holds_alternative<SendTelemetry>(action));

    REQUIRE(holds_alternative<Event>(action));
    REQUIRE(get<Event>(action).type == EventType::Info);

    // We'll succeed eventually
    event_info(planner);
    planner.action_done(ActionResult::Ok);

    // Goes back to telemetry after sucessful reinit
    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
    planner.action_done(ActionResult::Ok);
}

TEST_CASE("Unknown / broken command refused") {
    Planner planner;

    event_info(planner);
    planner.action_done(ActionResult::Ok);

    // Commands come as replies to telemetries
    Action action = planner.next_action();
    REQUIRE(holds_alternative<SendTelemetry>(action));
    planner.action_done(ActionResult::Ok);

    SECTION("Unknown") {
        planner.command(Command {
            1,
            CommandType::Unknown,
        });
    }

    SECTION("Broken") {
        planner.command(Command {
            1,
            CommandType::Broken,
        });
    }

    event_type(planner, EventType::Rejected);
    planner.action_done(ActionResult::Ok);
    REQUIRE(holds_alternative<SendTelemetry>(action));
    planner.action_done(ActionResult::Ok);
}

TEST_CASE("Send info request") {
    Planner planner;

    event_info(planner);
    planner.action_done(ActionResult::Ok);

    // Commands come as replies to telemetries
    Action action = planner.next_action();
    REQUIRE(holds_alternative<SendTelemetry>(action));
    planner.action_done(ActionResult::Ok);

    planner.command(Command {
        1,
        CommandType::SendInfo,
    });

    event_info(planner);
}

// TODO: Tests for unknown commands and such
