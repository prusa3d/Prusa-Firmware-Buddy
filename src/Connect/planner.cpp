#include "planner.hpp"
#include <timing.h>

#include <algorithm>
#include <cassert>

using std::min;
using std::nullopt;
using std::optional;

namespace con {

namespace {

    // A note about time comparisons. We usually subtract now() and some past
    // event, getting the length of the interval. This works fine around
    // wrap-around (because the subtraction will underflow and get to the low-ish
    // real number of milliseconds, which is fine).
    //
    // And our intervals are small. Things happen repeatedly under normal
    // circumstances. If we don't talk to the server for long enough, we schedule
    // an introduction Info event and after sending it, reset all the relevant time
    // values. We don't look at the intervals after the Info event was scheduled,
    // so the fact the intervals are long and might overflow/do weird things is of
    // no consequence.
    //
    // Yes, this is a bit subtle.
    //
    // All timestamps and durations are in milliseconds.

    // First retry after 100 ms.
    const constexpr Duration COOLDOWN_BASE = 100;
    // Don't do retries less often than once a minute.
    const constexpr Duration COOLDOWN_MAX = 1000 * 60;
    // Telemetry every 4 seconds. We may want to have something more clever later on.
    const constexpr Duration TELEMETRY_INTERVAL = 1000 * 4;
    // If we don't manage to talk to the server for this long, re-init the
    // communication with a new init event.
    const constexpr Duration RECONNECT_AFTER = 1000 * 10;

    // Just rename for better readability
    Timestamp now() {
        return ticks_ms();
    }

    optional<Duration> since(optional<Timestamp> past_event) {
        // optional::transform would be nice, but it's C++23
        if (past_event.has_value()) {
            // Underflow is OK here
            return now() - *past_event;
        } else {
            return nullopt;
        }
    }
}

const char *to_str(EventType event) {
    switch (event) {
    case EventType::Info:
        return "INFO";
    case EventType::Accepted:
        return "ACCEPTED";
    case EventType::Rejected:
        return "REJECTED";
    default:
        assert(false);
        return "???";
    }
}

void Planner::reset() {
    // TODO: Specific Info event
    planned_event = Event {
        EventType::Info,
    };
    last_telemetry = nullopt;
    cooldown = nullopt;
    perform_cooldown = false;
}

Action Planner::next_action() {
    if (perform_cooldown) {
        perform_cooldown = false;
        assert(cooldown.has_value());
        return Sleep {
            *cooldown
        };
    }

    if (planned_event.has_value()) {
        // We don't take it out yet. Only after it's successfuly sent.
        return *planned_event;
    }

    if (const auto since_telemetry = since(last_telemetry); since_telemetry.has_value()) {
        if (*since_telemetry >= TELEMETRY_INTERVAL) {
            return SendTelemetry { false };
        } else {
            // This shall not underflow, as the since_telemetry is small.
            return Sleep { TELEMETRY_INTERVAL - *since_telemetry };
        }
    } else {
        // TODO: Optimization: When can we send just empty telemetry instead of full one?
        return SendTelemetry { false };
    }
}

void Planner::action_done(ActionResult result) {
    switch (result) {
    case ActionResult::Refused:
        // In case of refused, we also remove the event, won't try to send it again.
    case ActionResult::Ok: {
        const Timestamp n = now();
        last_success = n;
        perform_cooldown = false;
        cooldown = nullopt;
        if (planned_event.has_value()) {
            planned_event = nullopt;
            // Enforce telemetry now. We may get a new command with it.
            last_telemetry = nullopt;
        } else {
            last_telemetry = n;
        }
        break;
    }
    case ActionResult::Failed:
        if (const auto since_success = since(last_success); since_success.value_or(0) >= RECONNECT_AFTER && !planned_event.has_value()) {
            // We have talked to the server long time ago (it's probably in
            // a galaxy far far away), so next time we manage to do so,
            // initialize the communication with the Info event again.

            planned_event = Event {
                EventType::Info,
            };
            last_success = nullopt;
        }

        // Failed to talk to the server. Retry after a while (with a back-off), but otherwise keep stuff the same.
        cooldown = min(COOLDOWN_MAX, cooldown.value_or(COOLDOWN_BASE / 2) * 2);
        perform_cooldown = true;
        break;
    }
}

void Planner::command(Command command) {
    // We can get commands only as result of telemetry, not of other things.
    // TODO: We probably want to have some more graceful way to deal with the
    // server sending us the command as a result to something else anyway.
    assert(!planned_event.has_value());
    switch (command.type) {
    case CommandType::Unknown:
    case CommandType::Broken:
        planned_event = Event {
            EventType::Rejected,
            command.id,
        };
        break;
    case CommandType::Gcode:
        // TODO: Implement
        planned_event = Event {
            EventType::Rejected,
            command.id,
        };
        break;
    case CommandType::SendInfo:
        planned_event = Event {
            EventType::Info,
            command.id,
        };
        break;
    }
}

}
