#pragma once

#include "changes.hpp"
#include "command.hpp"
#include "sleep.hpp"

#include <common/shared_buffer.hpp>
#include <transfers/monitor.hpp>
#include <transfers/download.hpp>
#include <transfers/changed_path.hpp>
#include <transfers/transfer.hpp>

#include <cstdint>
#include <optional>
#include <variant>

namespace http {
class Connection;
}

namespace connect_client {

class Printer;

struct SendTelemetry {
    enum class Mode {
        Reduced,
        Full,
    };
    Mode mode;
};

struct ReadCommand {};

enum class EventType {
    Info,
    JobInfo,
    FileInfo,
    FileChanged,
    TransferInfo,
    Rejected,
    Accepted,
    Finished,
    Failed,
    TransferStopped,
    TransferAborted,
    TransferFinished,
    CancelableChanged,
    StateChanged,
};

const char *to_str(EventType event);

struct Event {
    EventType type;
    std::optional<CommandId> command_id { std::nullopt };
    std::optional<uint16_t> job_id { std::nullopt };
    std::optional<SharedPath> path { std::nullopt };
    std::optional<transfers::TransferId> transfer_id { std::nullopt };
    /// Reason for the event. May be null.
    ///
    /// Reasons are constant strings, therefore the non-owned const char * ‒
    /// they are not supposed to get "constructed" or interpolated.
    const char *reason = nullptr;
    bool is_file = false;
    transfers::ChangedPath::Incident incident {};
    std::optional<CommandId> start_cmd_id { std::nullopt };
};

using Action = std::variant<
    SendTelemetry,
    Event,
    Sleep,
    ReadCommand,
    transfers::Download::InlineRequest>;

enum class ActionResult {
    Ok,
    Failed,
    Refused,
    RefusedFast,
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
    struct BackgroundCommand {
        CommandId id;
        BackgroundCmd command;
    };

    enum class TransferRecoveryState {
        WaitingForUSB,
        Finished,
    };

    Printer &printer;

    /// The next (or current) event we want to send out.
    std::optional<Event> planned_event;
    /// Last time we've successfully sent a telemetry to the server.
    std::optional<Timestamp> last_telemetry;
    /// When was the last time the telemetry was full?
    /// (0 can cause false negative at startup, but we also mark the telemetry tracker as dirty at startup).
    Timestamp last_full_telemetry = 0;
    SendTelemetry::Mode last_telemetry_mode = SendTelemetry::Mode::Reduced;
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

    /// How many times we've tried and failed due to some kind of network error
    /// or such? After enough, we give up sending a specific event because the
    /// failure might be related to that specific event in some way (in theory,
    /// it should not, they should be detected as Refused instead of Failed,
    /// but there are always corner cases).
    uint8_t failed_attempts = 0;

    /// Some command that is accepted but still being worked on.
    std::optional<BackgroundCommand> background_command;

    // Handlers for specific commands.
    void command(const Command &, const BrokenCommand &);
    void command(const Command &, const UnknownCommand &);
    void command(const Command &, const GcodeTooLarge &);
    void command(const Command &, const ProcessingThisCommand &);
    void command(const Command &, const ProcessingOtherCommand &);
    void command(const Command &, const Gcode &);
    void command(const Command &, const SendInfo &);
    void command(const Command &, const SendJobInfo &);
    void command(const Command &, const SendFileInfo &);
    void command(const Command &, const SendTransferInfo &);
    void command(const Command &, const PausePrint &);
    void command(const Command &, const ResumePrint &);
    void command(const Command &, const StopPrint &);
    void command(const Command &, const StartPrint &);
    void command(const Command &, const CancelPrinterReady &);
    void command(const Command &, const SetPrinterReady &);
    void command(const Command &, const SetPrinterIdle &);
    void command(const Command &, const StartEncryptedDownload &);
    void command(const Command &, const StartInlineDownload &);
    void command(const Command &, const DeleteFile &);
    void command(const Command &, const DeleteFolder &);
    void command(const Command &, const CreateFolder &);
    void command(const Command &, const StopTransfer &);
    void command(const Command &, const SetToken &);
    void command(const Command &, const ResetPrinter &);
    void command(const Command &, const SendStateInfo &);
    void command(const Command &, const DialogAction &);
    void command(const Command &, const SetValue &);
    void command(const Command &, const CancelObject &);
    void command(const Command &, const UncancelObject &);

    void handle_transfer_result(const Command &command, transfers::Transfer::BeginResult result);

    // Tracking if we should resend the INFO message due to some changes.
    Tracked info_changes;
    // Tracking how much we want to send.
    Tracked telemetry_changes;

    Tracked cancellable_objects;
    Tracked state_info;
    // Tracking of ongoing transfers.
    std::optional<transfers::TransferId> observed_transfer;

    // Tracks recovery of a transfer after startup.
    // We should not start any other transfer until the recovery is Finished.
    TransferRecoveryState transfer_recovery;

    // Set to true every time a transfer is observed. Also on startup (there
    // might be a leftover from before boot).
    //
    // Used to prevent cleaning up all the time.
    bool need_transfer_cleanup = true;

    /// Is there a command (possibly) waiting in the connection?
    ///
    /// (May contain true even if we lost the connection, or something like
    /// that. It also can be true for spurious wakeups that are at least
    /// theoretically possible in the API).
    bool command_waiting = false;

    // A transfer running in background.
    //
    // As we may have a background _task_ and a transfer at the same time, we
    // need to have variables for both.
    std::optional<transfers::Transfer> transfer;

    std::optional<CommandId> transfer_start_cmd = std::nullopt;
    std::optional<CommandId> print_start_cmd = std::nullopt;

    /// Constructs corresponding Sleep action.
    Sleep sleep(Duration duration, http::Connection *wake_on_readable, bool cooldown);

public:
    Planner(Printer &printer);
    /// Reset the state.
    ///
    /// This is for the "severe" cases, for example when connect is disabled
    /// and enabled or its configuration changes (and we may be possibly talking
    /// to another server).
    void reset();

    /// Reset the telemetry timeouts.
    ///
    /// For eg. reconnect on websocket, since the server node might want to
    /// have "fresh" telemetry.
    void reset_telemetry();
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
    Action next_action(SharedBuffer &buffer, http::Connection *wake_on_readable);
    /// Will we need the paths extracted from the current job?
    bool wants_job_paths() const;
    // Note: *Not* for Sleep. Only for stuff that sends.
    void action_done(ActionResult action);

    // Only for Success/Failure.
    void background_done(BackgroundResult result);
    void download_done(transfers::Transfer::State result);

    /// Called by sleep when it successfully decides whether there is a transfer
    /// to be recovered or not.
    void transfer_recovery_finished(std::optional<const char *> transfer_destination_path);

    void transfer_cleanup_finished(bool success);

    bool transfer_chunk(const transfers::Download::InlineChunk &chunk);
    void transfer_reset();

    // ID of a command being executed in the background, if any.
    std::optional<CommandId> background_command_id() const;

    /// Can we receive a new command right now?
    ///
    /// We _can't_ receive commands if we have an event scheduled to go out.
    bool can_receive_command() const;

    /// Inform the planner that there is (or may be) a command waiting.
    void notify_command_waiting() {
        command_waiting = true;
    }
};

} // namespace connect_client
