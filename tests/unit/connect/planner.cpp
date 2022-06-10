#include "time_mock.h"

#include <planner.hpp>

#include <catch2/catch.hpp>

using namespace con;
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

}

TEST_CASE("Success scenario") {
    Planner planner;

    REQUIRE(holds_alternative<Event>(planner.next_action()));
    planner.action_done(ActionResult::Ok);

    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
    planner.action_done(ActionResult::Ok);

    consume_sleep(planner);

    // But now, after we have slept, we shall send some more telemetries
    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
}

TEST_CASE("Retries early") {
    Planner planner;

    REQUIRE(holds_alternative<Event>(planner.next_action()));
    planner.action_done(ActionResult::Failed);

    // It'll provide a small sleep before trying again.
    Duration sleep1 = consume_sleep(planner);

    // Now it'll retry the event.
    REQUIRE(holds_alternative<Event>(planner.next_action()));
    planner.action_done(ActionResult::Failed);

    // If it fails again, it schedules a longer sleep.
    Duration sleep2 = consume_sleep(planner);
    REQUIRE(sleep1 < sleep2);

    // If we succeed after few retries, we move on to the telemetry
    REQUIRE(holds_alternative<Event>(planner.next_action()));
    planner.action_done(ActionResult::Ok);
    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
}

TEST_CASE("Reinit after several failures") {
    Planner planner;

    REQUIRE(holds_alternative<Event>(planner.next_action()));
    planner.action_done(ActionResult::Ok);

    // Eventually, it stops trying to send the telemetry and goes back to trying to send Info
    Action action;
    do {
        action = planner.next_action();
        planner.action_done(ActionResult::Failed);
        consume_sleep(planner);
    } while (holds_alternative<SendTelemetry>(action));

    REQUIRE(holds_alternative<Event>(action));

    // We'll succeed eventually
    REQUIRE(holds_alternative<Event>(planner.next_action()));
    planner.action_done(ActionResult::Ok);

    // Goes back to telemetry after sucessful reinit
    REQUIRE(holds_alternative<SendTelemetry>(planner.next_action()));
    planner.action_done(ActionResult::Ok);
}
