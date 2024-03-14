#include "planner.hpp"
#include "printer.hpp"

#include <filename_type.hpp>
#include <log.h>
#include <transfers/transfer.hpp>

#include <alloca.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <sys/stat.h>
#include "stat_retry.hpp"
#include <unistd.h>

using http::HeaderOut;
using std::holds_alternative;
using std::is_same_v;
using std::make_tuple;
using std::min;
using std::nullopt;
using std::optional;
using std::tuple;
using std::unique_ptr;
using std::visit;
using transfers::ChangedPath;
using transfers::Decryptor;
using transfers::Download;
using transfers::Monitor;
using transfers::Storage;
using transfers::Transfer;

using Type = ChangedPath::Type;
using Incident = ChangedPath::Incident;

LOG_COMPONENT_REF(connect);

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

    bool file_exists(const char *path) {
        struct stat st = {};
        if (stat_retry(path, &st) != 0) {
            return false;
        }

        if (S_ISDIR(st.st_mode)) {
            MutablePath mp(path);
            return transfers::is_valid_transfer(mp);
        }

        return true;
    }

    bool dir_exists(const char *path) {
        struct stat st = {};
        // This could give some false negatives, in practice rare (we don't have permissions, and such).
        return stat_retry(path, &st) == 0 and S_ISDIR(st.st_mode);
    }

    template <class>
    inline constexpr bool always_false_v = false;

    const char *delete_dir(const char *path) {
        int result = rmdir(path);
        if (result == -1) {
            if (errno == EACCES) {
                return "Directory not empty";
            } else {
                return "Error deleting directory";
            }
        }
        return nullptr;
    }

    const char *make_dir(const char *path) {
        if (mkdir(path, 777) != 0) {
            if (errno == EEXIST) {
                return "Directory already exists";
            } else {
                return "Error creating directory";
            }
        }

        return nullptr;
    }

    tuple<const char *, uint16_t> host_and_port(const Printer::Config &config, optional<uint16_t> port_override) {
        uint16_t port = config.port;
        if (port == 443 && config.tls) {
            // Go from encrypted to the unencrypted port automatically.
            port = 80;
        }
        if (port_override.has_value()) {
            // Manual override always takes precedence.
            port = *port_override;
        }
        const char *host = config.host;

        return make_tuple(host, port);
    }

    constexpr const char *const enc_prefix = "/f/";
    constexpr const char *const enc_suffix = "/raw";
    const size_t enc_prefix_len = strlen(enc_prefix);
    const size_t enc_suffix_len = strlen(enc_suffix);
    const size_t iv_len = 2 /* Binary->hex conversion*/ * StartEncryptedDownload::BLOCK_SIZE;
    const size_t enc_url_len = enc_prefix_len + enc_suffix_len + iv_len + 1;

    void make_enc_url(char *buffer /* assumed to be at least enc_url_len large */, const Decryptor::Block &iv) {
        strcpy(buffer, enc_prefix);

        for (size_t i = 0; i < iv.size(); i++) {
            sprintf(buffer + enc_prefix_len + 2 * i, "%02hhx", iv[i]);
        }

        buffer[enc_prefix_len + 2 * iv.size()] = '\0';
        strcat(buffer, enc_suffix);
    }

    Transfer::BeginResult init_transfer(Printer &, const Printer::Config &config, const StartEncryptedDownload &download) {
        const char *dpath = download.path.path();
        if (!path_allowed(dpath)) {
            return Storage { "Not allowed outside /usb" };
        }

        if (!filename_is_firmware(dpath) && !filename_is_printable(dpath)) {
            return Storage { "Unsupported file type" };
        }

        const auto [host, port] = host_and_port(config, download.port);

        char *path = nullptr;
        unique_ptr<Download::EncryptionInfo> encryption;

        path = reinterpret_cast<char *>(alloca(enc_url_len));
        make_enc_url(path, download.iv);
        encryption = make_unique<Download::EncryptionInfo>(download.key, download.iv, download.orig_size);

        auto request = Download::Request(host, port, path, std::move(encryption));

        return Transfer::begin(dpath, request);
    }
} // namespace

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
    case EventType::FileChanged:
        return "FILE_CHANGED";
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

Sleep Planner::sleep(Duration amount, bool cooldown) {
    // Note for the case where planned_event.has_value():
    //
    // Processing of background command could generate another event that
    // would overwrite this one, which we don't want. We want to send that one
    // out first.
    //
    // Why are we sleeping anyway? Because we have trouble sending it?
    const bool has_event = planned_event.has_value();
    BackgroundCmd *cmd = (background_command.has_value() && !has_event) ? &background_command->command : nullptr;
    // This is not the case for downloads, download-finished events are sent by
    // "passively" watching what is or is not being transferred and the event
    // is generated after the fact anyway. No reason to block downloading for
    // that.
    Transfer *down = transfer.has_value() ? &transfer.value() : nullptr;
    // we don't want to allow moving the gcode file whenever there is a chance
    // something is touching it (though it would fail anyway) and while we need
    // the performance for other things (especially the USB performance).
    //
    // We also don't want to allow it during downloading.
    bool allow_transfer_cleanup = !printer.is_printing() && need_transfer_cleanup && !Monitor::instance.id().has_value();
    return Sleep(
        amount,
        cmd,
        down,
        /*run_transfer_cleanup=*/allow_transfer_cleanup,
        /* The cooldown thing: We don't want to recover transfer before we get properly connected, maybe we don't have the IP yet or something. */
        /*run_transfer_recovery=*/(transfer_recovery != TransferRecoveryState::Finished) && !cooldown);
}

Action Planner::next_action(SharedBuffer &buffer) {
    if (!printer.is_printing()) {
        // The idea is, we set the ID when we start the print and remove it
        // once we see we are no longer printing. This is not completely
        // correct, because:
        //
        // * A print can end and a new one start (without using Connect)
        //   between two calls to next_action, not resetting the command as
        //   necessary.
        // * On the other hand, currently we _probably_ can reach some state
        //   that is not considered "printing" while really printing (eg. the
        //   Busy state in crash detection, maybe?), in which case we reset it
        //   even if we shouldn't.
        // * We don't keep this info across a power panic.
        //
        // Nevertheless, this has low impact. Connect asks for JOB_INFO at the
        // first opportunity it sees a new job, to know if it may remove it
        // from the queue. In the first case, it would have nothing to remove
        // (done in the previous job), and the latter likely doesn't happen
        // because it asks at the beginning and has it already.
        //
        // Finding a 100% correct tracking for this would be really complex,
        // because the start of the print is asynchronous (we don't get an
        // answer from the marlin right away), we don't know at that point what
        // job ID we'll have, we don't get notifications about terminated
        // prints, etc. Out of the just-slightly broken solutions, this one
        // seems the simplest.
        print_start_cmd.reset();
    }

    if (perform_cooldown) {
        perform_cooldown = false;
        assert(cooldown.has_value());
        return sleep(*cooldown, true);
    }

    if (planned_event.has_value()) {
        // We don't take it out yet. Only after it's successfuly sent.
        return *planned_event;
    }

    if (info_changes.set_hash(printer.info_fingerprint())) {
        planned_event = Event {
            EventType::Info,
        };
        return *planned_event;
    }

    auto current_transfer = Monitor::instance.id();

    if (current_transfer.has_value()) {
        // Some transfer (maybe even link one) is running, so it might need cleaning up afterwards.
        need_transfer_cleanup = true;
    }

    if (auto current_transfer = Monitor::instance.id(); observed_transfer != current_transfer) {
        auto terminated_transfer = observed_transfer;
        optional<Monitor::Outcome> outcome = terminated_transfer.has_value() ? Monitor::instance.outcome(*terminated_transfer) : nullopt;

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
            // Not nullopt, otherwise we wouldn't get an outcome.
            planned_event->transfer_id = *terminated_transfer;
            planned_event->start_cmd_id = transfer_start_cmd;
            transfer_start_cmd = nullopt;
            return *planned_event;
        }
        // No info:
        // * It may be out of history
        // * Or there was no transfer to start with, we are changing from nullopt
    }

    auto changed_path_status = ChangedPath::instance.status();
    if (changed_path_status.has_value()) {
        auto &changed_path = *changed_path_status;
        auto buff(buffer.borrow());
        if (buff.has_value()) {
            changed_path.consume(reinterpret_cast<char *>(buff->data()), buff->size());

            EventType type = (changed_path.is_file() && changed_path.what_happend() == Incident::Created) ? EventType::FileInfo : EventType::FileChanged;
            planned_event = Event {
                type,
                changed_path.triggered_command_id(),
                nullopt,
                SharedPath(std::move(*buff)),
            };
            planned_event->is_file = changed_path.is_file();
            planned_event->incident = changed_path.what_happend();
            return *planned_event;
        }
    }

    if (const auto since_telemetry = since(last_telemetry); since_telemetry.has_value()) {
        const Duration telemetry_interval = printer.is_printing() || background_command.has_value() ? TELEMETRY_INTERVAL_SHORT : TELEMETRY_INTERVAL_LONG;
        if (*since_telemetry >= telemetry_interval) {
            return SendTelemetry { false };
        } else {
            Duration sleep_amount = telemetry_interval - *since_telemetry;
            return sleep(sleep_amount, false);
        }
    } else {
        // TODO: Optimization: When can we send just empty telemetry instead of full one?
        return SendTelemetry { false };
    }
}

bool Planner::wants_job_paths() const {
    return planned_event.has_value() && planned_event->type == EventType::JobInfo;
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
    } else if (!file_exists(path)) {
        reason = "File not found";
    } else if (!printer.start_print(path)) {
        reason = "Can't print now";
    }

    if (reason == nullptr) {
        print_start_cmd = command.id;
        planned_event = Event { EventType::JobInfo, command.id };
        // Note: We let job_id be empty here and that disables the "check" for
        // the same job.
        planned_event->start_cmd_id = command.id;
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
    planned_event->start_cmd_id = print_start_cmd;
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

void Planner::command(const Command &command, const SendTransferInfo &) {
    planned_event = Event {
        EventType::TransferInfo,
        command.id,
    };
    planned_event->start_cmd_id = transfer_start_cmd;
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

void Planner::command(const Command &, const ProcessingThisCommand &) {
    // Unreachable:
    // * In case we are processing this command, this is handled one level up
    //   (because we don't want to hit the safety checks there).
    // * It can't happen to be generated when we are _not_ processing a
    //   background command.
    assert(0);
}

void Planner::command(const Command &command, const StartEncryptedDownload &download) {
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

    if (transfer_recovery == TransferRecoveryState::WaitingForUSB) {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Not ready" };
        return;
    }

    auto down_result = init_transfer(printer, config, download);

    visit([&](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (is_same_v<T, transfers::Transfer>) {
            // If there was another download, it wouldn't have succeeded
            // because it wouldn't acquire the transfer slot.
            assert(!this->transfer.has_value());

            this->transfer = Transfer { std::move(arg) };
            planned_event = Event { EventType::TransferInfo, command.id };
            planned_event->start_cmd_id = command.id;
            transfer_start_cmd = command.id;
        } else if constexpr (is_same_v<T, transfers::NoTransferSlot>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Another transfer in progress" };
        } else if constexpr (is_same_v<T, transfers::AlreadyExists>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "File already exists" };
        } else if constexpr (is_same_v<T, transfers::RefusedRequest>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Failed to download" };
        } else if constexpr (is_same_v<T, transfers::Storage>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, arg.msg };
        } else {
            static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
    },
        down_result);
}

void Planner::command(const Command &command, const DeleteFile &params) {
    const char *path = params.path.path();

    const char *reason = nullptr;
    if (!path_allowed(path)) {
        reason = "Forbidden path";
    } else if (!file_exists(path)) {
        reason = "File not found";
    } else if (auto err = printer.delete_file(path); err != nullptr) {
        reason = err;
    }

    if (reason == nullptr) {
        ChangedPath::instance.changed_path(path, Type::File, Incident::Deleted, command.id);
        // The "result" is generated through the FILE_CHANGED event indirectly,
        // so not setting it here.
    } else {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, reason };
    }
}

void Planner::command(const Command &command, const DeleteFolder &params) {
    const char *path = params.path.path();

    const char *reason = nullptr;
    if (!path_allowed(path)) {
        reason = "Forbidden path";
    } else if (!dir_exists(path)) {
        reason = "File not found";
    } else if (auto err = delete_dir(path); err != nullptr) {
        reason = err;
    }

    if (reason == nullptr) {
        ChangedPath::instance.changed_path(path, Type::Folder, Incident::Deleted, command.id);
        // The "result" is generated through the FILE_CHANGED event indirectly,
        // so not setting it here.
    } else {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, reason };
    }
}

void Planner::command(const Command &command, const CreateFolder &params) {
    const char *path = params.path.path();

    const char *reason = nullptr;
    if (!path_allowed(path)) {
        reason = "Forbidden path";
    } else if (auto err = make_dir(path); err != nullptr) {
        reason = err;
    }

    if (reason == nullptr) {
        ChangedPath::instance.changed_path(path, Type::Folder, Incident::Created, command.id);
        // The "result" is generated through the FILE_CHANGED event indirectly,
        // so not setting it here.
    } else {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, reason };
    }
}

void Planner::command(const Command &command, [[maybe_unused]] const StopTransfer &params) {
    const char *reason = nullptr;
    if (!Monitor::instance.signal_stop()) {
        reason = "No transfer in progress";
    }
    if (reason == nullptr) {
        planned_event = { EventType::Finished, command.id };
    } else {
        planned_event = { EventType::Rejected, command.id, nullopt, nullopt, nullopt, reason };
    }
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

optional<CommandId> Planner::background_command_id() const {
    if (background_command.has_value()) {
        return background_command->id;
    } else {
        return nullopt;
    }
}

void Planner::background_done(BackgroundResult result) {
    // Function contract, caller not supposed to supply anything else.
    assert(result == BackgroundResult::Success || result == BackgroundResult::Failure);
    // We give out the background task only as part of a sleep and we do so
    // only in case we don't have an event to be sent out.
    assert(!planned_event.has_value());
    // Obviously, it can be done only in case there's one.
    assert(background_command.has_value());
    planned_event = Event {
        result == BackgroundResult::Success ? EventType::Finished : EventType::Failed,
        background_command_id(),
    };
    background_command.reset();
}

void Planner::transfer_recovery_finished(std::optional<const char *> transfer_destination_path) {
    need_transfer_cleanup = true;
    transfer_recovery = TransferRecoveryState::Finished;
    if (!transfer_destination_path.has_value()) {
        log_info(connect, "No transfer to recover");
        return;
    }

    log_info(connect, "Recovering transfer %s", *transfer_destination_path);

    auto recovery_result = Transfer::recover(*transfer_destination_path);
    std::visit([&](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Transfer>) {
            transfer = std::move(arg);
        } else if constexpr (std::is_same_v<T, transfers::NoTransferSlot>) {
            // this should not happen as we don't allow to start transfers before we finish recovery
            log_error(connect, "No transfer slot available for recovery of a transfer");
        } else if constexpr (std::is_same_v<T, transfers::Storage>) {
            // not really something we can do about this I guess
            log_error(connect, "Storage error during recovery of a transfer: %s", arg.msg);
        } else {
            static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
    },
        recovery_result);
}

void Planner::download_done(Transfer::State result) {
    // Similar reasons as with background_done
    log_info(connect, "Download done, result %i", static_cast<int>(result));
    assert(transfer.has_value());

    // We do _not_ set the event here. We do so in watching the transfer.
    //
    // But we make sure the observed_transfer is set even if there was no
    // next_event in the meantime or if it was short-circuited.

    observed_transfer = Monitor::instance.id();
    assert(observed_transfer.has_value()); // Because download still holds the slot.
    transfer.reset();
}

void Planner::transfer_cleanup_finished(bool success) {
    // Retry in case of failure.
    need_transfer_cleanup = !success;
}

} // namespace connect_client
