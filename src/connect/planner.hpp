#pragma once

#include "buffer.hpp"
#include "command.hpp"
#include "printer.hpp"

#include <cstdint>
#include <optional>
#include <variant>

namespace connect_client {

// Just make the code a bit more readable by making this distinction.
// Unfortunately, not checked at compile time.
using Timestamp = uint32_t;
using Duration = uint32_t;

struct Sleep {
    Duration milliseconds;
};

struct SendTelemetry {
    bool empty;
};

enum class EventType {
    Info,
    JobInfo,
    FileInfo,
    Rejected,
    Accepted,
    Finished,
};

const char *to_str(EventType event);

struct Event {
    EventType type;
    std::optional<CommandId> command_id;
    std::optional<uint16_t> job_id;
    std::optional<SharedPath> path;
};

using Action = std::variant<
    Sleep,
    SendTelemetry,
    Event>;

enum class ActionResult {
    Ok,
    Failed,
    Refused,
};

/// The planner
///
/// This part is responsible for coordinating what happens when. As we have
/// only one connection to the Connect servers, we can issue only one request at
/// a time. We need to decide which one to do right now (if any) and react to
/// commands from the server.
///
/// It is also responsible for doing retries, re-initializing communication and
/// similar after something bad happens.
class Planner {
private:
    Printer &printer;

    /// The next (or current) event we want to send out.
    std::optional<Event> planned_event;
    /// Last time we've successfully sent a telemetry to the server.
    std::optional<Timestamp> last_telemetry;
    /// Last time we've successfully talked to the server.
    std::optional<Timestamp> last_success;
    /// When doing comm retries, this is the cooldown time between them.
    ///
    /// In case we are in a unsuccessfull row, this is keeping the current
    /// value, which gets incremented each time (up to a limit).
    std::optional<Duration> cooldown;
    /// The next action shall be a cooldown.
    ///
    /// Note that this might be set to false and still have the above cooldown
    /// set. In that case we just did the cooldown, can retry right now, but we
    /// want to keep the value in case the retry also fails.
    bool perform_cooldown;

    // Handlers for specific commands.
    void command(const Command &, const BrokenCommand &);
    void command(const Command &, const UnknownCommand &);
    void command(const Command &, const ProcessingOtherCommand &);
    void command(const Command &, const Gcode &);
    void command(const Command &, const SendInfo &);
    void command(const Command &, const SendJobInfo &);
    void command(const Command &, const SendFileInfo &);
    void command(const Command &, const PausePrint &);
    void command(const Command &, const ResumePrint &);
    void command(const Command &, const StopPrint &);
    void command(const Command &, const StartPrint &);

public:
    Planner(Printer &printer)
        : printer(printer) {
        reset();
    }
    /// Reset the state.
    ///
    /// This is for the "severe" cases, for example when connect is disabled
    /// and enabled or its configuration changes (and we may be possibly talking
    /// to another server).
    void reset();
    /// A command was received from the server.
    ///
    /// The caller is responsible only for handing it over. No automatic acks
    /// or rejects or handling on the caller side is expected.
    void command(Command command);
    /// What should the caller do next.
    ///
    /// Returns the next action to take. It may be a sleep (in which case the
    /// caller may take a nap but in case something happens ‒ eg. a command
    /// arrives, it might interact sooner).
    ///
    /// All actions except sleeps expect a follow-up call to action_done.
    Action next_action();
    // Note: *Not* for Sleep. Only for stuff that sends.
    void action_done(ActionResult action);
};

}
