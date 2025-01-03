#include "planner.hpp"
#include "netdev.h"
#include "printer.hpp"

#include <path_utils.h>
#include <filename_type.hpp>
#include <logging/log.hpp>
#include <transfers/transfer.hpp>
#include <option/websocket.h>
#include <general_response.hpp>
#include <wui.h>
#include <netif_settings.h>
#include <config_store/store_instance.hpp>
#if XL_ENCLOSURE_SUPPORT()
    #include <xl_enclosure.hpp>
#endif
#if PRINTER_IS_PRUSA_COREONE() || defined(UNITTESTS)
    #include <feature/chamber/chamber.hpp>
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
    #include <feature/xbuddy_extension/cooling.hpp>
#endif
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
    // Don't send telemetry more often than this even if things change.
    const constexpr Duration TELEMETRY_INTERVAL_MIN = 750;
#if WEBSOCKET()
    // Max of 2 minutes of telemetry silence.
    const constexpr Duration TELEMETRY_INTERVAL_LONG = 2 * 60 * 1000;
#else
    // Telemetry every 4 seconds. We may want to have something more clever later on.
    const constexpr Duration TELEMETRY_INTERVAL_LONG = 1000 * 4;
#endif
    // Except when we are printing or processing something, we want it more often.
    const constexpr Duration TELEMETRY_INTERVAL_SHORT = 1000;
    // Make sure to send a full telemetry once in a while, even if there are no
    // relevant changes. That's because the server might forget the telemetry sometimes.
    const constexpr Duration TELEMETRY_INTERVAL_FULL = 1000 * 60 * 5;
    // Wake the loop at least this often, to check for interesting events. This
    // limits the max amoun of one sleep, but we'd produce several in a row if
    // we need that.
    const constexpr Duration LOOP_WAKEUP = 500;
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
        if (mkdir(path, 0777) != 0) {
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

    Transfer::BeginResult init_transfer(const StartInlineDownload &download) {
        const char *dpath = download.path.path();
        if (!path_allowed(dpath)) {
            return Storage { "Not allowed outside /usb" };
        }

        if (!filename_is_transferrable(dpath)) {
            return Storage { "Unsupported file type" };
        }

        auto request = Download::Request(download.hash, download.team_id, download.orig_size);

        return Transfer::begin(dpath, request);
    }

    Transfer::BeginResult init_transfer(const Printer::Config &config, const StartEncryptedDownload &download) {
        const char *dpath = download.path.path();
        if (!path_allowed(dpath)) {
            return Storage { "Not allowed outside /usb" };
        }

        if (!filename_is_transferrable(dpath)) {
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

    bool command_is_error_whitelisted(const Command &command) {
        return holds_alternative<SendInfo>(command.command_data) || holds_alternative<SetToken>(command.command_data) || holds_alternative<ResetPrinter>(command.command_data) || holds_alternative<SendStateInfo>(command.command_data);
    }

    const char *set_hostname(const char *new_hostname) {
        if (strlen(new_hostname) > HOSTNAME_LEN) {
            return "Hostname too long";
        }

        if (strcmp(config_store().hostname.get_c_str(), new_hostname) != 0) {
            log_info(connect, "Changing hostname to: %s", new_hostname);
            config_store().hostname.set(new_hostname);
            notify_reconfigure();
        }
        return nullptr;
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
    case EventType::CancelableChanged:
        return "CANCELABLE_CHANGED";
    case EventType::StateChanged:
        return "STATE_CHANGED";
    default:
        assert(false);
        return "???";
    }
}

const char *to_str(MachineReason reason) {
    switch (reason) {
    case MachineReason::None:
        // Not actually rendered, just to cover the case.
        return "";
    case MachineReason::TransferInProgress:
        return "TRANSFER_IN_PROGRESS";
    case MachineReason::FileExists:
        return "FILE_EXISTS";
    case MachineReason::StorageFailure:
        return "STORAGE_FAILURE";
    case MachineReason::NotReady:
        return "NOT_READY";
    case MachineReason::NetworkFailure:
        return "NETWORK_FAILURE";
    }

    assert(false);
    return "???";
}

Planner::Planner(Printer &printer)
    : printer(printer) {
    reset();

    // Prevent emiting a state changed to IDLE on boot.
    Printer::Params dummy_params(nullopt);
    dummy_params.state.device_state = printer_state::DeviceState::Idle;
    state_info.set_hash(dummy_params.state_fingerprint());
    state_info.mark_clean();
    telemetry_changes.mark_dirty();
}

void Planner::reset() {
    // Will trigger an Info message on the next one.
    info_changes.mark_dirty();
    cancellable_objects.mark_clean();
    last_telemetry = nullopt;
    telemetry_changes.mark_dirty();
    cooldown = nullopt;
    perform_cooldown = false;
    failed_attempts = 0;
}

void Planner::reset_telemetry() {
    last_telemetry = nullopt;
    telemetry_changes.mark_dirty();
}

Sleep Planner::sleep(Duration amount, http::Connection *wake_on_readable, bool cooldown) {
    if (!can_receive_command()) {
        // We don't want to cut the sleep short in case there is a command waiting but we can't receive it.
        wake_on_readable = nullptr;
    }

    // Don't do anything "extra" during bluescreen/redscreen.
    if (printer.is_in_error()) {
        return Sleep(amount, nullptr, nullptr, wake_on_readable, false, false);
    }
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
        wake_on_readable,
        /*run_transfer_cleanup=*/allow_transfer_cleanup,
        /* The cooldown thing: We don't want to recover transfer before we get properly connected, maybe we don't have the IP yet or something. */
        /*run_transfer_recovery=*/(transfer_recovery != TransferRecoveryState::Finished) && !cooldown);
}

Action Planner::next_action(SharedBuffer &buffer, http::Connection *wake_on_readable) {
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
        return sleep(*cooldown, nullptr, true);
    }

    if (planned_event.has_value()) {
        // We don't take it out yet. Only after it's successfuly sent.
        return *planned_event;
    }

    printer.set_can_start_download(transfer_recovery != TransferRecoveryState::WaitingForUSB);
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
            MachineReason reason = MachineReason::None;

            switch (*outcome) {
            case Monitor::Outcome::Finished:
                type = EventType::TransferFinished;
                break;
            case Monitor::Outcome::ErrorStorage:
                type = EventType::TransferAborted;
                reason = MachineReason::StorageFailure;
                break;
            case Monitor::Outcome::ErrorNetwork:
                type = EventType::TransferAborted;
                reason = MachineReason::NetworkFailure;
                break;
            case Monitor::Outcome::ErrorOther:
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
            planned_event->machine_reason = reason;
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

    const auto params = printer.params();

    if (state_info.set_hash(params.state_fingerprint())) {
        planned_event = Event {
            EventType::StateChanged,
        };
        return *planned_event;
    }

    if (cancellable_objects.set_hash(printer.cancelable_fingerprint())) {
        planned_event = Event {
            EventType::CancelableChanged,
        };
        return *planned_event;
    }

    if (command_waiting && can_receive_command()) {
        // It's OK to reset it here without any success notification from
        // outside. That's because it'll get set again in the nearest sleep if
        // we lose it.
        command_waiting = false;
        return ReadCommand {};
    }

    const bool printing = printer.is_printing();
    const bool changes = telemetry_changes.set_hash(params.telemetry_fingerprint(!printing));
    const bool want_short = printing || background_command.has_value() || observed_transfer.has_value();
    const Duration telemetry_interval = want_short ? TELEMETRY_INTERVAL_SHORT : TELEMETRY_INTERVAL_LONG;
    const Duration since_telemetry = since(last_telemetry).value_or(TELEMETRY_INTERVAL_FULL * 2 /* "Long enough" */);
    const Duration since_full = since(last_full_telemetry).value_or(TELEMETRY_INTERVAL_FULL * 2);

    const bool send_telemetry = since_telemetry >= TELEMETRY_INTERVAL_MIN && (changes || since_telemetry >= telemetry_interval);
    const bool want_full = changes || since_full >= TELEMETRY_INTERVAL_FULL;

    if (!send_telemetry && transfer.has_value() && transfer->download.has_value()) {
        // This call "consumes" the request, so we won't use it next time.
        // Nevertheless, if the connection fails (we are unable to deliver it),
        // the old download is discarded anyway and a new one is born.
        if (auto request = transfer->download->inline_request(); request.has_value()) {
            return *request;
        }
    }

    if (send_telemetry) {
        last_telemetry_mode = want_full ? SendTelemetry::Mode::Full : SendTelemetry::Mode::Reduced;
        return SendTelemetry { last_telemetry_mode };
    } else {
        // Don't sleep longer than until the next telemetry.
        // But also wake up often enough to check for interesting events.
        assert(telemetry_interval >= since_telemetry);
        Duration sleep_amount = std::min(telemetry_interval - since_telemetry, LOOP_WAKEUP);
        return sleep(sleep_amount, wake_on_readable, false);
    }
}

bool Planner::wants_job_paths() const {
    return planned_event.has_value() && planned_event->type == EventType::JobInfo;
}

bool Planner::can_receive_command() const {
    return !planned_event.has_value();
}

void Planner::action_done(ActionResult result) {
    auto exponential_backoff = [&]() {
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
    };

    auto reset_backoff = [&]() {
        perform_cooldown = false;
        cooldown = nullopt;
    };

    auto cleanups = [&]() {
        // In case of refused, we also remove the event, won't try to send it again.
        failed_attempts = 0;

        if (planned_event.has_value()) {
            if (planned_event->type == EventType::Info) {
                info_changes.mark_clean();
            } else if (planned_event->type == EventType::CancelableChanged) {
                cancellable_objects.mark_clean();
            } else if (planned_event->type == EventType::StateChanged) {
                state_info.mark_clean();
            }
            // Enforce telemetry now. We may get a new command with it.
            // Websocket still need this, even tho commands can come independently from telemetry,
            // because the telemetry might change as a result of the event, that was finished.
            last_telemetry = nullopt;
        } else {
            const Timestamp n = now();
            last_telemetry = n;
            if (last_telemetry_mode == SendTelemetry::Mode::Full) {
                last_full_telemetry = n;
                telemetry_changes.mark_clean();
            }
        }
        planned_event.reset();
    };

    switch (result) {
    case ActionResult::Refused:
        cleanups();
        exponential_backoff();
        break;
    case ActionResult::RefusedFast:
        cleanups();
        reset_backoff();
        break;
    case ActionResult::Ok: {
        const Timestamp n = now();
        last_success = n;
        reset_backoff();
        cleanups();
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
                cleanups();
            }
            failed_attempts = 0;
        }

        exponential_backoff();
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
    } else if (!transfers::is_valid_file_or_transfer(path)) {
        reason = "File not found";
    } else if (const char *error = printer.start_print(path, params.tool_mapping); error != nullptr) {
        reason = error;
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

void Planner::command(const Command &command, const SetPrinterIdle &) {
    auto result = printer.set_idle() ? EventType::Finished : EventType::Rejected;
    const char *reason = (result == EventType::Rejected) ? "Can't set idle now" : nullptr;
    planned_event = Event { result, command.id, nullopt, nullopt, nullopt, reason };
}

void Planner::command(const Command &, const ProcessingThisCommand &) {
    // Unreachable:
    // * In case we are processing this command, this is handled one level up
    //   (because we don't want to hit the safety checks there).
    // * It can't happen to be generated when we are _not_ processing a
    //   background command.
    assert(0);
}

void Planner::handle_transfer_result(const Command &command, Transfer::BeginResult result) {
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
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Another transfer in progress", MachineReason::TransferInProgress };
        } else if constexpr (is_same_v<T, transfers::AlreadyExists>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "File already exists", MachineReason::FileExists };
        } else if constexpr (is_same_v<T, transfers::Storage>) {
            planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, arg.msg, MachineReason::StorageFailure };
        } else {
            static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
    },
        result);
}

void Planner::command(const Command &command, const StartInlineDownload &download) {
    if (transfer_recovery == TransferRecoveryState::WaitingForUSB) {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Not ready" };
        return;
    }

    auto down_result = init_transfer(download);

    handle_transfer_result(command, std::move(down_result));
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
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Not ready", MachineReason::NotReady };
        return;
    }

    auto down_result = init_transfer(config, download);

    handle_transfer_result(command, std::move(down_result));
}

void Planner::command(const Command &command, const DeleteFile &params) {
    const char *path = params.path.path();

    const char *reason = nullptr;
    if (!path_allowed(path)) {
        reason = "Forbidden path";
    } else if (!transfers::is_valid_file_or_transfer(path)) {
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

void Planner::command(const Command &command, const SetToken &params) {
    printer.init_connect(reinterpret_cast<const char *>(params.token->data()));
    planned_event = { EventType::Finished, command.id };
}

void Planner::command(const Command &command, const ResetPrinter &) {
    printer.reset_printer();

    // We reach this place only if the reset_printer fails to execute (can it?)
    planned_event = { EventType::Rejected, command.id, nullopt, nullopt, nullopt, "Failed to reset" };
}

void Planner::command(const Command &command, const SendStateInfo &) {
    planned_event = { EventType::StateChanged, command.id };
}

void Planner::command(const Command &command, const DialogAction &params) {
    if (const char *error = printer.dialog_action(params.dialog_id, params.response); error != nullptr) {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, error };
    } else {
        planned_event = { EventType::Finished, command.id };
    }
}

void Planner::command(const Command &command, const SetValue &params) {
    const char *err = nullptr;

    auto adjust_nozzle = [&](size_t idx, auto cback) {
        auto slot = printer.params().slots[idx];
        cback(slot);
        printer.set_slot_info(idx, slot);
    };

    switch (params.name) {
    case connect_client::PropertyName::HostName:
        err = set_hostname(reinterpret_cast<const char *>(get<SharedBorrow>(params.value)->data()));
        break;
#if XL_ENCLOSURE_SUPPORT()
    case connect_client::PropertyName::EnclosureEnabled:
        xl_enclosure.setEnabled(get<bool>(params.value));
        break;
    case connect_client::PropertyName::EnclosurePrintingFiltration:
        xl_enclosure.setPrintFiltration(get<bool>(params.value));
        break;
    case connect_client::PropertyName::EnclosurePostPrint:
        xl_enclosure.setPostPrintFiltration(get<bool>(params.value));
        break;
    case connect_client::PropertyName::EnclosurePostPrintFiltrationTime: {
        // we recieve it in seconds, but this function expects minutes
        uint32_t raw_value = get<uint32_t>(params.value);
        uint32_t minutes = raw_value / 60;
        if (raw_value % 60 != 0) {
            err = "Value should be whole minutes";
        } else if (minutes >= 1 && minutes <= 10) {
            xl_enclosure.setPostPrintFiltrationDuration(minutes);
        } else {
            err = "Value out of range";
        }
        break;
    }
#endif
    case connect_client::PropertyName::NozzleHighFlow:
        adjust_nozzle(params.idx, [&](auto &slot) {
            slot.high_flow = get<bool>(params.value);
        });
        break;
    case connect_client::PropertyName::NozzleHardened:
        adjust_nozzle(params.idx, [&](auto &slot) {
            slot.hardened = get<bool>(params.value);
        });
        break;
    case connect_client::PropertyName::NozzleDiameter:
        adjust_nozzle(params.idx, [&](auto &slot) {
            slot.nozzle_diameter = get<float>(params.value);
        });
        break;
#if PRINTER_IS_PRUSA_COREONE() || defined(UNITTESTS)
    case connect_client::PropertyName::ChamberTargetTemp: {
        auto target_temp = get<uint32_t>(params.value);
        buddy::chamber().set_target_temperature(target_temp == connect_client::Printer::ChamberInfo::target_temp_unset ? nullopt : std::make_optional(target_temp));
    } break;
    case connect_client::PropertyName::ChamberFanPwmTarget: {
        int8_t pwm = get<int8_t>(params.value);
        if (pwm < 0) {
            buddy::xbuddy_extension().set_fan1_fan2_auto_control();
        } else {
            buddy::xbuddy_extension().set_fan1_fan2_pwm(buddy::FanCooling::pct2pwm(pwm)); // convert from percentage to PWM
        }
    } break;
    case connect_client::PropertyName::ChamberLedIntensity:
        buddy::xbuddy_extension().set_chamber_leds_pwm(buddy::XBuddyExtension::led_pct2pwm(get<int8_t>(params.value)));
        break;
    case connect_client::PropertyName::AddonPower:
        buddy::xbuddy_extension().set_usb_power(get<bool>(params.value));
        break;
#endif
    }
    if (err != nullptr) {
        planned_event = Event { EventType::Rejected, command.id, nullopt, nullopt, nullopt, err };
    } else {
        planned_event = { EventType::Finished, command.id };
    }
}

#if ENABLED(CANCEL_OBJECTS)
void Planner::command(const Command &command, const CancelObject &params) {
    printer.cancel_object(params.id);
    // Reset the hash to the current (modified) cancel mask.
    // We don't need to do .renew on the printer, the marlin vars are propagated "instantly"
    cancellable_objects.set_hash(printer.cancelable_fingerprint());
    // We confirm the command by sending the current cancellable state
    // (even if it didn't change by this modification - like if it was already canceled, etc)
    planned_event = Event {
        EventType::CancelableChanged,
        command.id,
    };
}

void Planner::command(const Command &command, const UncancelObject &params) {
    printer.uncancel_object(params.id);
    // Reset the hash to the current (modified) cancel mask.
    // We don't need to do .renew on the printer, the marlin vars are propagated "instantly"
    cancellable_objects.set_hash(printer.cancelable_fingerprint());
    // We confirm the command by sending the current cancellable state
    // (even if it didn't change by this modification - like if it was already canceled, etc)
    planned_event = Event {
        EventType::CancelableChanged,
        command.id,
    };
}
#else
void Planner::command(const Command &command, const CancelObject &) {
    planned_event = Event {
        EventType::Rejected,
        command.id,
    };
    planned_event->reason = "Not supported on this printer type";
}

void Planner::command(const Command &command, const UncancelObject &) {
    planned_event = Event {
        EventType::Rejected,
        command.id,
    };
    planned_event->reason = "Not supported on this printer type";
}
#endif

// FIXME: Handle the case when we are resent a command we are already
// processing for a while. In that case, we want to re-Accept it. Nevertheless,
// we may not be able to parse it again because the background command might be
// holding the shared buffer. Therefore, this must happen on some higher level?
void Planner::command(Command command) {
    // We can get commands only as result of telemetry, not of other things.
    // TODO: We probably want to have some more graceful way to deal with the
    // server sending us the command as a result to something else anyway.

    assert(!planned_event.has_value());
    if (printer.is_in_error() && !command_is_error_whitelisted(command)) {
        planned_event = Event {
            EventType::Rejected,
            command.id,
        };
        planned_event->reason = "Won't accept commands in error state";
        return;
    }

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

bool Planner::transfer_chunk(const Download::InlineChunk &chunk) {
    if (transfer.has_value() && transfer->download.has_value()) {
        return transfer->download->inline_chunk(chunk);
    } else {
        // If we have no transfer, it is probably just a stray chunk from
        // before aborting. Blackhole it silently, but don't kill the
        // connection.
        return true;
    }
}

void Planner::transfer_reset() {
    if (transfer.has_value()) {
        transfer->recoverable_failure(printer.is_printing());
    }
}

} // namespace connect_client
