#include "planner.hpp"
#include <timing.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <sys/stat.h>

using std::holds_alternative;
using std::min;
using std::nullopt;
using std::optional;
using std::visit;
using transfers::Download;
using transfers::Monitor;

namespace connect_client {

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
    const constexpr Duration TELEMETRY_INTERVAL_LONG = 1000 * 4;
    // Except when we are printing or processing something, we want it more often.
    const constexpr Duration TELEMETRY_INTERVAL_SHORT = 1000;
    // If we don't manage to talk to the server for this long, re-init the
    // communication with a new init event.
    const constexpr Duration RECONNECT_AFTER = 1000 * 10;

    // Max number of attempts per specific event before we throw it out of the
    // window. Safety measure, as it may be related to that specific event and
    // we would never recover if the failure is repeateble with it.
    const constexpr uint8_t GIVE_UP_AFTER_ATTEMPTS = 5;

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

    bool path_allowed(const char *path) {
        constexpr const char *const usb = "/usb/";
        // Note: allow even "bare" /usb
        const bool is_on_usb = strncmp(path, usb, strlen(usb)) == 0 || strcmp(path, "/usb") == 0;
        const bool contains_upper = strstr(path, "/../") != nullptr;
        return is_on_usb && !contains_upper;
    }

    bool path_exists(const char *path) {
        struct stat st = {};
        // This could give some false negatives, in practice rare (we don't have permissions, and such).
        return stat(path, &st) == 0;
    }

    template <class>
    inline constexpr bool always_false_v = false;
}

const char *to_str(EventType event) {
    switch (event) {
    case EventType::Info:
        return "INFO";
    case EventType::Accepted:
        return "ACCEPTED";
    case EventType::Rejected:
        return "REJECTED";
    case EventType::JobInfo:
        return "JOB_INFO";
    case EventType::FileInfo:
        return "FILE_INFO";
    case EventType::TransferInfo:
        return "TRANSFER_INFO";
    case EventType::Finished:
        return "FINISHED";
    case EventType::Failed:
        return "FAILED";
    case EventType::TransferStopped:
        return "TRANSFER_STOPPED";
    case EventType::TransferAborted:
        return "TRANSFER_ABORTED";
    case EventType::TransferFinished:
        return "TRANSFER_FINISHED";
    default:
        assert(false);
        return "???";
    }
}

void Planner::reset() {
    // Will trigger an Info message on the next one.
    info_changes.mark_dirty();
    last_telemetry = nullopt;
    cooldown = nullopt;
    perform_cooldown = false;
    failed_attempts = 0;
}

Action Planner::next_action() {
    if (perform_cooldown) {
        perform_cooldown = false;
        assert(cooldown.has_value());
        Duration bt = background_processing(*cooldown);
        return Sleep {
            *cooldown - bt
        };
    }

    if (planned_event.has_value()) {
        // We don't take it out yet. Only after it's successfuly sent.
        return *planned_event;
    }

    if (info_changes.set_hash(printer.info_fingerprint()) || file_changes.set_hash(printer.files_hash())) {
        planned_event = Event {
            EventType::Info,
        };
        if (file_changes.is_dirty()) {
            planned_event->info_rescan_files = true;
        }
        return *planned_event;
    }

    if (auto current_transfer = Monitor::instance.id(); observed_transfer != current_transfer) {
        optional<Monitor::Outcome> outcome = observed_transfer.has_value() ? Monitor::instance.outcome(*observed_transfer) : nullopt;

        auto terminated_transfer = *observed_transfer;
        observed_transfer = current_transfer;

        if (outcome.has_value()) {
            // The default value will never be used, it
            // is set only to shut up the compiler about
            // uninitialized use
            EventType type = EventType::Failed;

            switch (*outcome) {
            case Monitor::Outcome::Finished:
                type = EventType::TransferFinished;
                break;
            case Monitor::Outcome::Error:
                type = EventType::TransferAborted;
                break;
            case Monitor::Outcome::Stopped:
                type = EventType::TransferStopped;
                break;
            }
            planned_event = Event {
                type,
            };
            planned_event->transfer_id = terminated_transfer;
            return *planned_event;
        }
        // No info:
        // * It may be out of history
        // * Or there was no transfer to start with, we are changing from nullopt
    }

    if (const auto since_telemetry = since(last_telemetry); since_telemetry.has_value()) {
        const Duration telemetry_interval = printer.is_printing() || background_command.has_value() ? TELEMETRY_INTERVAL_SHORT : TELEMETRY_INTERVAL_LONG;
        if (*since_telemetry >= telemetry_interval) {
            return SendTelemetry { false };
        } else {
            Duration sleep_amount = telemetry_interval - *since_telemetry;
            Duration bt = background_processing(sleep_amount);
            if (planned_event.has_value()) {
                // A new event appeared as part of the background
                // processing, that one takes precedence!
                return next_action();
            } else {
                // This shall not underflow, as the since_telemetry is small.
                return Sleep { sleep_amount - bt };
            }
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
        failed_attempts = 0;
        if (planned_event.has_value()) {
            if (planned_event->type == EventType::Info) {
                info_changes.mark_clean();
                if (planned_event->info_rescan_files) {
                    file_changes.mark_clean();
                }
            }
            planned_event = nullopt;
            // Enforce telemetry now. We may get a new command with it.
            last_telemetry = nullopt;
        } else {
            last_telemetry = n;
        }
        break;
    }
    case ActionResult::Failed:
        if (++failed_attempts >= GIVE_UP_AFTER_ATTEMPTS) {
            // Give up after too many failed attemts when trying to send the
            // same thing. The failure may be related to the specific event in
            // some way (we have seen a "payload too large" error from the
            // server, for example, which, due to our limitations, we are
            // unable to distinguish from just a network error while sending
            // the data), so avoid some kind of infinite loop/blocked state.
            if (planned_event.has_value() && planned_event->type != EventType::Info) {
                planned_event.reset();
            }
            failed_attempts = 0;
        }

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

void Planner::command(const Command &command, const UnknownCommand &) {
    planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Unknown command" };
}

void Planner::command(const Command &command, const BrokenCommand &c) {
    planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, c.reason };
}

void Planner::command(const Command &command, const GcodeTooLarge &) {
    planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "GCode too large" };
}

void Planner::command(const Command &command, const ProcessingOtherCommand &) {
    planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Processing other command" };
}

void Planner::command(const Command &command, const Gcode &gcode) {
    background_command = BackgroundCommand {
        command.id,
        BackgroundGcode {
            gcode.gcode,
            gcode.size,
            0,
        },
    };
    planned_event = Event { EventType::Accepted, command.id };
}

#define JC(CMD, REASON)                                                                                   \
    void Planner::command(const Command &command, const CMD##Print &) {                                   \
        if (printer.job_control(Printer::JobControl::CMD)) {                                              \
            planned_event = Event { EventType::Finished, command.id };                                    \
        } else {                                                                                          \
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, REASON }; \
        }                                                                                                 \
    }

JC(Pause, "No print to pause")
JC(Resume, "No paused print to resume")
JC(Stop, "No print to stop")

void Planner::command(const Command &command, const StartPrint &params) {
    const char *path = params.path.path();

    const char *reason = nullptr;
    if (!path_allowed(path)) {
        reason = "Forbidden path";
    } else if (!path_exists(path)) {
        reason = "File not found";
    } else if (!printer.start_print(path)) {
        reason = "Can't print now";
    }

    if (reason == nullptr) {
        planned_event = Event { EventType::Finished, command.id };
    } else {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, reason };
    }
}

void Planner::command(const Command &command, const SendInfo &) {
    planned_event = Event {
        EventType::Info,
        command.id,
    };
}

void Planner::command(const Command &command, const SendJobInfo &params) {
    planned_event = Event {
        EventType::JobInfo,
        command.id,
        params.job_id,
    };
}

void Planner::command(const Command &command, const SendFileInfo &params) {
    if (path_allowed(params.path.path())) {
        planned_event = Event {
            EventType::FileInfo,
            command.id,
            nullopt, // job_id
            params.path,
        };
    } else {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Forbidden path" };
    }
}

void Planner::command(const Command &command, const SendTransferInfo &params) {
    planned_event = Event {
        EventType::TransferInfo,
        command.id,
    };
}

void Planner::command(const Command &command, const SetPrinterReady &) {
    auto result = printer.set_ready(true) ? EventType::Finished : EventType::Rejected;
    const char *reason = (result == EventType::Rejected) ? "Can't set ready now" : nullptr;
    planned_event = Event { result, command.id, nullopt, nullopt, nullopt, reason };
}

void Planner::command(const Command &command, const CancelPrinterReady &) {
    bool ok = printer.set_ready(false);
    // Setting _not_ ready can't fail.
    assert(ok);
    (void)ok; // Avoid warnging when asserts are disabled.
    planned_event = Event { EventType::Finished, command.id };
}

void Planner::command(const Command &command, const ProcessingThisCommand &) {
    // Unreachable:
    // * In case we are processing this command, this is handled one level up
    //   (because we don't want to hit the safety checks there).
    // * It can't happen to be generated when we are _not_ processing a
    //   background command.
    assert(0);
}

void Planner::command(const Command &command, const StartConnectDownload &download) {
    // Get the config (we need it for the connection); don't reset the "changed" flag.
    auto [config, config_changed] = printer.config(false);
    if (config_changed) {
        // If the config changed, there's a chance the old server send us a
        // command to download stuff and we would download it from the new one,
        // which a) wouldn't have it, b) we could leak some info to the new
        // server we are not supposed to. Better safe than sorry.
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Switching config" };
        return;
    }

    if (config.tls) {
        // TODO Once we have the support for symmetric encryption, we refuse
        // this only if we have no decryption key ready.
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Encryption of downloads not supported" };
        return;
    }

    // TODO: Support overriding port:
    // By a field in the message
    // Going from 443 to 80 on TLS connections
    const uint16_t port = config.port;
    const char *host = config.host;

    const char *prefix = "/p/teams/";
    const char *infix = "/files/";
    const char *suffix = "/raw";
    const size_t buffer_len = strlen(prefix) + 21 /* max len of 64bit number */ + strlen(infix) + strlen(download.hash) + strlen(suffix) + 1;
    char path[buffer_len];
    size_t written = snprintf(path, buffer_len, "%s%" PRIu64 "%s%s%s", prefix, download.team, infix, download.hash, suffix);
    // Written is number of chars that _would_ be written if there's enough
    // space, which means that if it's size or longer, we got it truncated.
    //
    // That would mean we somehow miscalculated the buffer estimate.
    assert(written < buffer_len);
    // Avoid warning about unused in release builds (assert off)
    (void)written;

    auto down_result = Download::start_connect_download(host, port, path, download.path.path());

    visit([&](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, transfers::Download>) {
            // If there was another download, it wouldn't have succeeded
            // because it wouldn't acquire the transfer slot.
            assert(!this->download.has_value());

            this->download = std::move(arg);
            planned_event = Event { EventType::Finished, command.id };
        } else if constexpr (std::is_same_v<T, transfers::NoTransferSlot>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Another transfer in progress" };
        } else if constexpr (std::is_same_v<T, transfers::AlreadyExists>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "File already exists" };
        } else if constexpr (std::is_same_v<T, transfers::RefusedRequest>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Failed to download" };
        } else if constexpr (std::is_same_v<T, transfers::Storage>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, arg.msg };
        } else {
            static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
    },
        down_result);

    // TODO: This is temporary. As we don't do the actual download, so release the slot.
    this->download.reset();
}

// FIXME: Handle the case when we are resent a command we are already
// processing for a while. In that case, we want to re-Accept it. Nevertheless,
// we may not be able to parse it again because the background command might be
// holding the shared buffer. Therefore, this must happen on some higher level?
void Planner::command(Command command) {
    // We can get commands only as result of telemetry, not of other things.
    // TODO: We probably want to have some more graceful way to deal with the
    // server sending us the command as a result to something else anyway.
    assert(!planned_event.has_value());

    if (background_command.has_value()) {
        // We are already processing a command.
        // If it's this particular one, we just continue processing it and re-accept it.
        planned_event = Event {
            holds_alternative<ProcessingThisCommand>(command.command_data) ? EventType::Accepted : EventType::Rejected,
            command.id,
        };
        return;
    }

    visit([&](const auto &arg) {
        this->command(command, arg);
    },
        command.command_data);
}

Planner::BackgroundResult Planner::background_task(BackgroundGcode &gcode) {
    if (gcode.size <= gcode.position) {
        // FIXME: We need a way to know if the commands are not just submitted,
        // but also processed. Some kind of additional "cork" command that'll
        // report back to us and we can check it, maybe?

        // All gcode submitted.
        return BackgroundResult::Success;
    }

    // In C++, it's a lot of work to convert void * -> char * or uint8_t * ->
    // char *, although it's both legal conversion (at least in this case). In
    // C, that works out of the box without casts.
    const char *start = reinterpret_cast<const char *>(gcode.data->data()) + gcode.position;
    const size_t tail_size = gcode.size - gcode.position;

    const char *newline = reinterpret_cast<const char *>(memchr(start, '\n', tail_size));
    // If there's no newline at all, pretend that there's one just behind the end.
    const size_t end_pos = newline != nullptr ? newline - start : tail_size;

    // We'll replace the \n with \0
    char gcode_buf[end_pos + 1];
    memcpy(gcode_buf, start, end_pos);
    gcode_buf[end_pos] = '\0';

    // Skip whitespace at the start and the end
    // (\0 isn't a space, so it works as a stop implicitly)
    char *g_start = gcode_buf;
    while (isspace(*g_start)) {
        g_start++;
    }

    char *g_end = g_start + strlen(g_start) - 1;
    while (g_end >= g_start && isspace(*g_end)) {
        *g_end = '\0';
        g_end--;
    }

    // Skip over empty ones to not hog the queue
    if (strlen(g_start) > 0) {
        // FIXME: This can block if the queue is full.
        printer.submit_gcode(g_start);
    }

    gcode.position += end_pos + 1;

    return BackgroundResult::More;
}

Duration Planner::background_processing(Duration limit) {
    if (background_command.has_value()) {
        Timestamp start = now();

        BackgroundResult last_result;
        do {
            last_result = visit([&](auto &arg) {
                return this->background_task(arg);
            },
                background_command->command);

            if (last_result == BackgroundResult::Success || last_result == BackgroundResult::Failure) {
                planned_event = Event {
                    last_result == BackgroundResult::Success ? EventType::Finished : EventType::Failed,
                    background_command->id,
                };
                background_command = nullopt;
            }

            if (last_result == BackgroundResult::More && since(start) > limit) {
                last_result = BackgroundResult::Later;
            }
        } while (last_result == BackgroundResult::More);

        // Note: we always pass value in, so we get one out.
        return min(limit, since(start).value());
    } else {
        return 0;
    }
}

optional<CommandId> Planner::background_command_id() const {
    if (background_command.has_value()) {
        return background_command->id;
    } else {
        return nullopt;
    }
}

}
