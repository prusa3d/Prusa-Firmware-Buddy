#include "transfer.hpp"
#include "changed_path.hpp"
#include "download.hpp"

// TODO: use slot and poll from gui?
#include "gui/gui_media_events.hpp"

#include <common/timing.h>
#include <common/crc32.h>
#include <common/filename_type.hpp>
#include <common/bsod.h>
#include <common/unique_dir_ptr.hpp>
#include <common/print_utils.hpp>
#include <common/stat_retry.hpp>
#include <common/lfn.h>
#include <common/scope_guard.hpp>
#include <state/printer_state.hpp>
#include <option/has_human_interactions.h>

#include <logging/log.hpp>
#include <type_traits>
#include <variant>
#include <optional>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

LOG_COMPONENT_REF(transfers);

using namespace transfers;
using std::is_same_v;
using std::optional;

Transfer::PlainGcodeDownloadOrder::PlainGcodeDownloadOrder(const PartialFile &file) {
    if (file.has_valid_tail(TailSize)) {
        state = State::DownloadingBody;
    } else {
        state = State::DownloadingTail;
    }
}

Transfer::Action Transfer::PlainGcodeDownloadOrder::step(const PartialFile &file) {
    switch (state) {
    case State::DownloadingTail:
        if (file.has_valid_tail(TailSize)) {
            state = State::DownloadingBody;
            return Action::RangeJump;
        }
        return Action::Continue;
    case State::DownloadingBody:
        if (file.final_size() == file.get_state().get_valid_size()) {
            return Action::Finished;
        } else {
            return Action::Continue;
        }
    default:
        fatal_error("unhandled state", "download");
    }
}

size_t Transfer::PlainGcodeDownloadOrder::get_next_offset(const PartialFile &file) const {
    switch (state) {
    case State::DownloadingBody: {
        auto head = file.get_valid_head();
        return head.has_value() ? head->end : 0;
    }
    case State::DownloadingTail: {
        auto tail = file.get_valid_tail();
        log_info(transfers, "returning offset for tail: %i, %u, %u", tail.has_value(), tail->start, tail->end);
        return tail.has_value() ? tail->end : file.final_size() - TailSize;
    }
    default:
        fatal_error("unhandled state", "download");
    }
}

Transfer::Action Transfer::GenericFileDownloadOrder::step(const PartialFile &file) {
    if (file.final_size() == file.get_state().get_valid_size()) {
        return Action::Finished;
    } else {
        return Action::Continue;
    }
}

size_t Transfer::GenericFileDownloadOrder::get_next_offset(const PartialFile &file) const {
    auto head = file.get_valid_head();
    return head ? head->end : 0;
}

Transfer::BeginResult Transfer::begin(const char *destination_path, const Download::Request &request) {
    log_info(transfers, "Starting transfer of %s", destination_path);

    // allocate slot for the download
    auto slot = Monitor::instance.allocate(Monitor::Type::Connect, destination_path, 0, true);
    if (!slot.has_value()) {
        log_error(transfers, "Failed to allocate slot for %s", destination_path);
        return NoTransferSlot {};
    }

    // check the destination path does not exist
    struct stat st;
    if (stat_retry(destination_path, &st) == 0) {
        log_error(transfers, "Destination path %s already exists", destination_path);
        return AlreadyExists {};
    }

    auto path = Path(destination_path);
    unique_file_ptr backup;
    PartialFile::Result preallocated;
    PartialFile::Ptr partial_file;

    ScopeGuard cleanup([&]() {
        // Close all potentially opened files
        backup.reset();
        preallocated = "";
        partial_file.reset();
        // And remove everything we may have created
        remove(path.as_partial());
        remove(path.as_backup());
        rmdir(path.as_destination());
    });

    // make a directory there
    if (mkdir(destination_path, 0777) != 0) {
        log_error(transfers, "Failed to create directory %s", destination_path);
        return Storage { "Failed to create directory" };
    }

    if (!store_transfer_index(destination_path)) {
        log_error(transfers, "Failed to store path to index");
        return Storage { "Failed to store path to index" };
    }

    // make the request
    backup = unique_file_ptr(fopen(path.as_backup(), "w+"));
    if (backup.get() == nullptr) {
        return Storage { "Failed to create backup file" };
    }
    size_t file_size = request.orig_size();
    preallocated = move(PartialFile::create(path.as_partial(), file_size));
    if (const char **err = get_if<const char *>(&preallocated); err != nullptr) {
        const char *e = *err; // Backup, cleanup resets preallocated state
        return Storage { e };
    };
    partial_file = get<PartialFile::Ptr>(move(preallocated));
    if (Transfer::make_backup(backup.get(), request, partial_file->get_state(), *slot) == false) {
        return Storage { "Failed to fill the backup file" };
    }

    cleanup.disarm();
    backup.reset();
    slot->update_expected_size(file_size);

    Transfer transfer(std::move(*slot), partial_file);
    transfer.restart_download();
    return transfer;
}

bool Transfer::restart_download() {
    auto backup_file = unique_file_ptr(fopen(path.as_backup(), "r"));
    if (backup_file.get() == nullptr) {
        log_error(transfers, "Failed to open backup file");
        last_connection_error_ms = ticks_ms();
        return false;
    }

    auto backup = Transfer::restore(backup_file.get());
    if (backup.has_value() == false) {
        log_error(transfers, "Failed to restore backup file");
        last_connection_error_ms = ticks_ms();
        return false;
    }

    auto request = backup->get_download_request();
    if (request.has_value() == false) {
        log_error(transfers, "Failed to get download request from backup file");
        last_connection_error_ms = ticks_ms();
        return false;
    }

    init_download_order_if_needed();

    // We try to reinicialize the PartialFile, in case the USB got re-plugged or something.
    const size_t check_size = partial_file->final_size();
    const PartialFile::State old_state = partial_file->get_state();
    // We can't really deallocate it completely (if we do next
    // restart_download, we need to keep the state and size), but we want to
    // make sure we don't hold the file actually open so the next open can
    // succeed.
    partial_file->release_file();
    if (auto open_result = PartialFile::open(path.as_partial(), old_state, true); holds_alternative<PartialFile::Ptr>(open_result)) {
        auto new_file = move(get<PartialFile::Ptr>(open_result));
        if (new_file->final_size() != check_size) {
            return false;
        }
        partial_file = move(new_file);
    } else {
        return false;
    }

    uint32_t position = std::visit([&](auto &&arg) { return arg.get_next_offset(*partial_file); }, *order);
    position = position / PartialFile::SECTOR_SIZE * PartialFile::SECTOR_SIZE; // ensure we start at a sector boundary

    optional<uint32_t> end_range;
    if (auto tail = partial_file->get_valid_tail(); tail.has_value()) {
        if (tail->end == partial_file->final_size() && position < tail->start) {
            // We can request not until the end of file, but until the
            // beginning of the tail - we'll stop there and have the complete
            // file by then (the tail is already all the way to the end).
            //
            // Note: end_range is _inclusive_ in the http (eg. range 0-4 will
            // return 5 bytes).
            assert(tail->start % PartialFile::SECTOR_SIZE == 0);
            end_range = tail->start - 1;
        }
    }

    download.emplace(*request, partial_file, position, end_range);

    return true;
}

void Transfer::init_download_order_if_needed() {
    if (order.has_value()) {
        return;
    }
    bool is_plain_gcode = filename_is_plain_gcode(slot.destination());
    bool has_sufficient_size = partial_file->final_size() >= PlainGcodeDownloadOrder::MinimalFileSize;
    if (is_plain_gcode && has_sufficient_size) {
        order = DownloadOrder(PlainGcodeDownloadOrder(*partial_file));
    } else {
        order = DownloadOrder(GenericFileDownloadOrder());
    }
}

void Transfer::update_backup(bool force) {
    const auto state = partial_file->get_state();
    const size_t size = state.get_valid_size();
    const bool backup_outdated = last_backup_update_ms.has_value() == false || ticks_ms() - *last_backup_update_ms > BackupUpdateIntervalMs;
    const bool did_cross_size = size - last_backup_update_bytes > BackupUpdateIntervalBytes;
    if (force == false && !backup_outdated && !did_cross_size) {
        log_debug(transfers, "Not updating backup file (%zu confirmed, %zu downloaded)", last_backup_update_bytes, size);
        return;
    }

    unique_file_ptr backup_file(fopen(path.as_backup(), "r+"));
    if (backup_file.get() == nullptr) {
        log_error(transfers, "Failed to open backup file for update");
        return;
    }

    if (Transfer::update_backup(backup_file.get(), partial_file->get_state()) == false) {
        log_error(transfers, "Failed to update backup file");
    } else {
        log_info(transfers, "Backup file updated at %zu", size);
    }
    last_backup_update_ms = ticks_ms();
    last_backup_update_bytes = size;

    if (is_printable && !already_notified) {
        notify_created();
    }
}

std::optional<struct stat> Transfer::get_transfer_partial_file_stat(MutablePath &destination_path) {
    const auto r = transfer_check(destination_path, TransferCheckValidOnly::yes);
    if (!r.is_valid()) {
        return std::nullopt;
    }

    return r.partial_file_stat;
}

Transfer::RecoverResult Transfer::recover(const char *destination_path) {
    Path path(destination_path);

    std::optional<RestoredTransfer> backup = std::nullopt;

    PartialFile::State partial_file_state;
    {
        auto backup_file = unique_file_ptr(fopen(path.as_backup(), "r"));
        if (backup_file.get() == nullptr) {
            log_error(transfers, "Failed to open backup file");
            return Storage { "Failed to open backup file" };
        }

        backup = Transfer::restore(backup_file.get());
        if (backup.has_value() == false) {
            log_error(transfers, "Failed to restore backup file, invalidating transfer");
            // Mark it as failed and it'll get cleaned up soon
            // (so the user can try re-uploading it, for example)
            backup_file.reset();
            unique_file_ptr invalidate_backup(fopen(path.as_backup(), "w"));
            return Storage { "Failed to restore backup file" };
        }
        partial_file_state = backup->get_partial_file_state();
    }

    // reopen the partial file
    PartialFile::Ptr partial_file = nullptr;
    {
        auto partial_file_result = PartialFile::open(path.as_partial(), partial_file_state, true);
        if (auto *err = get_if<const char *>(&partial_file_result); err != nullptr) {
            log_error(transfers, "Failed to open partial file: %s", *err);
            return Storage { *err };
        } else {
            partial_file = std::get<PartialFile::Ptr>(partial_file_result);
        }
    }

    // allocate slot for the transfer
    auto slot = Monitor::instance.allocate(Monitor::Type::Connect, destination_path, partial_file->final_size(), false, backup->id);
    if (!slot.has_value()) {
        log_error(transfers, "Failed to allocate slot for %s", destination_path);
        return NoTransferSlot {};
    }

    slot->progress(partial_file_state, false);

    return Transfer(std::move(*slot), partial_file);
}

Transfer::Transfer(Monitor::Slot &&slot, PartialFile::Ptr partial_file)
    : slot(std::move(slot))
    , path(slot.destination())
    , state(State::Retrying)
    , partial_file(partial_file)
    , is_printable(filename_is_printable(slot.destination())) {
    assert(partial_file.get() != nullptr);
}

Transfer::State Transfer::step(bool is_printing) {
    switch (state) {
    case State::Downloading:
    case State::Retrying: {
        if (slot.is_stopped()) {
            done(State::Failed, Monitor::Outcome::Stopped);
        } else if (download.has_value()) {
            auto step_result = download->step();
            bool has_issues = step_result != DownloadStep::Continue && step_result != DownloadStep::Finished;
            auto state = partial_file->get_state();
            auto downloaded_size = state.get_valid_size();
            if (downloaded_size != last_downloaded_size) {
                // If we make any progress, reset the retries.
                // (progress is updated whenever the USB submits a new block).
                last_downloaded_size = downloaded_size;
                retries_left = MAX_RETRIES;
            }
            slot.progress(state, has_issues);
            if (has_issues) {
                update_backup(/*force=*/true);
            } else {
                init_download_order_if_needed();
                Transfer::Action next_step = std::visit([&](auto &&arg) { return arg.step(*partial_file); }, *order);

                switch (next_step) {
                case Transfer::Action::Continue:
                    break;
                case Transfer::Action::RangeJump:
                    download.reset();
                    // So we don't "lose" part of the already downloaded file, for showing on screen, etc.
                    update_backup(/*force=*/true);
                    restart_requested_by_jump = true;
                    break;
                case Transfer::Action::Finished:
                    done(State::Finished, Monitor::Outcome::Finished);
                    // With the plain gcodes where we download out of order, it
                    // may happen that we already have the whole file, but the
                    // download would still be able to provide some more data
                    // and would say Continue. Fix that situation up here
                    // (especially because we don't want to touch the now
                    // thrown away partial file).
                    step_result = DownloadStep::Finished;
                    break;
                }
            }
            switch (step_result) {
            case DownloadStep::Continue:
                update_backup(/*force=*/false);
                break;
            case DownloadStep::FailedNetwork:
                recoverable_failure(is_printing);
                break;
            case DownloadStep::FailedStorage:
                done(State::Failed, Monitor::Outcome::ErrorStorage);
                break;
            case DownloadStep::FailedRemote:
                done(State::Failed, Monitor::Outcome::ErrorOther);
                break;
            case DownloadStep::Finished:
                download.reset();
                break;
            case DownloadStep::Aborted:
                // Unreachable - this is only after we've called the deleter
                assert(0);
                break;
            }
        } else if (last_connection_error_ms.has_value() == false || ticks_ms() - *last_connection_error_ms > 1000) {
            slot.progress(partial_file->get_state(), !restart_requested_by_jump);
            restart_requested_by_jump = false;
            if (!restart_download()) {
                // OK, some of them are probably not recoverable (eg. someone
                // has eaten the backup file at runtime), but also not expected
                // to generally happen in practice, so it's probably fine to
                // just try multiple times in that case before giving up
                // completely.
                recoverable_failure(is_printing);
            }
        }
        break;
    }
    case State::Finished:
    case State::Failed:
        break;
    }
    return state;
}

void Transfer::notify_created() {
    ChangedPath::instance.changed_path(slot.destination(), ChangedPath::Type::File, ChangedPath::Incident::CreatedEarly);

    if (HAS_HUMAN_INTERACTIONS() && filename_is_printable(slot.destination()) && printer_state::remote_print_ready(/*preview_only=*/true)) {
        // While it looks a counter-intuitive, this print_begin only shows the
        // print preview / one click print, doesn't really start the print.
        char sfn_path[FILE_PATH_BUFFER_LEN];
        get_SFN_path_copy(slot.destination(), sfn_path, sizeof(sfn_path));
        print_begin(sfn_path);
    }

    already_notified = true;
}

void Transfer::notify_success() {
    ChangedPath::instance.changed_path(slot.destination(), ChangedPath::Type::File, ChangedPath::Incident::Created);
}

bool Transfer::cleanup_transfers() {
    auto index = unique_file_ptr(fopen(transfer_index, "r"));
    if (!index) {
        // No index means nothing to clean up, which is successful.
        return true;
    }

    Path transfer_path;

    bool all_ok = true;
    bool can_cleanup = true;

    for (;;) {
        switch (next_in_index(index, transfer_path)) {
        case IndexIter::Ok: {
            MutablePath mp(transfer_path.as_destination());
            const auto r = transfers::transfer_check(mp);

            if (r.is_running()) {
                can_cleanup = false;
            } else if (r.is_aborted() && !cleanup_remove(transfer_path)) {
                all_ok = false;
            } else if (r.is_finished() && !cleanup_finalize(transfer_path)) {
                all_ok = false;
            }

            break;
        }
        case IndexIter::Skip:
            continue;
        case IndexIter::IndividualError:
            all_ok = false;
            continue;
        case IndexIter::FatalError:
            all_ok = false;
            [[fallthrough]];
        case IndexIter::Eof:
            goto DONE; // break, but from the cycle
        }
    }

DONE:
    if (all_ok && can_cleanup) {
        // Close file so we can remove it.
        //
        // Note: There's a short race condition - if between we close it and
        // delete it, another transfer starts in Link and gets written in the
        // file, we lose it (once Link also starts using partial files). That's
        // probably rare and not a catastrophic failure.
        index.reset();
        remove(transfer_index);
    }

    return all_ok;
}

void Transfer::recoverable_failure(bool is_printing) {
    if (retries_left > 0) {
        log_warning(transfers, "Network failure, %zu retries left", retries_left);
        if (!is_printing) {
            // We want to make sure not to give up on downloading
            // the file that is being printed. This is much broader
            // (we won't give up on downloading some other
            // completely unrelated file too), but that's probably
            // fine and we don't want the complexity of plumbing
            // all the details about what is being printed, what is
            // being downloaded and if these are in fact the same
            // files (considering every segment of the path might
            // be either LFN or SFN).
            retries_left--;
        }
        slot.progress(partial_file->get_state(), true);
        state = State::Retrying;
        restart_requested_by_jump = false;
        download.reset();
    } else {
        done(State::Failed, Monitor::Outcome::ErrorNetwork);
    }
}

void Transfer::done(State state, Monitor::Outcome outcome) {
    this->state = state;
    download.reset();
    if (!partial_file) {
        // Done called multiple times.
        return;
    }
    partial_file.reset();
    if (state == State::Finished) {
        remove(path.as_backup());
        // If the file is being printed or manipulated in some other way,
        // this'll fail (because an open file can't be moved). That's OK, we
        // still have a full cleanup planned when the printer is idle. But we
        // want to try this as early as possible anyway.
        if (!cleanup_finalize(path)) {
            // If moving to place suceeds, it already contains that notification.
            //
            // If it fails (because we are printing it), we still want to send
            // the notification out (and again, with changed read-only, once
            // the cleanup happens).
            notify_success();
        }
    } else {
        // FIXME: We need some kind of error handling strategy to deal with
        // failed transfers. But for now, we just need to make 100% sure
        // not to mark the download as „successfully“ finished. So we mark
        // it as failed by having an empty backup file.

        // (Overwrite the file with empty one by opening and closing right away).
        unique_file_ptr(fopen(path.as_backup(), "w"));

        // And try to clean it up if possible. Might fail if it is being
        // printed or for some similar reasons (again, that's fine, we'll try
        // to do cleanup later too).
        //
        // Unlike the success, we do not early-notify the removal.
        cleanup_remove(path);
    }
    slot.done(outcome);

    log_info(transfers, "Transfer %s", state == State::Failed ? "failed" : "finished");
}

bool Transfer::cleanup_finalize(Path &transfer_path) {
    // move the partial file to temporary location
    const char *temporary_filename = "/usb/prusa-temporary-file.gcode";
    remove(temporary_filename); // remove the file if there is some leftover already

    char SFN[FILE_PATH_BUFFER_LEN];
    strlcpy(SFN, transfer_path.as_destination(), sizeof(SFN));
    get_SFN_path(SFN);
    uint32_t old_SFN_crc = crc32_calc((const uint8_t *)SFN, sizeof(SFN));
    if (rename(transfer_path.as_partial(), temporary_filename) != 0) {
        log_error(transfers, "Failed to move partial file to temporary location");
        return false;
    }
    // remove the transfer directory
    if (rmdir(transfer_path.as_destination()) != 0) {
        log_error(transfers, "Failed to remove transfer directory");
        return false;
    }
    if (rename(temporary_filename, transfer_path.as_destination()) != 0) {
        log_error(transfers, "Failed to move temporary file to final location");
        return false;
    }

    strlcpy(SFN, transfer_path.as_destination(), sizeof(SFN));
    get_SFN_path(SFN);
    uint32_t new_SFN_crc = crc32_calc((const uint8_t *)SFN, sizeof(SFN));

    if (old_SFN_crc != new_SFN_crc) {
        // if SFN changed, trigger a rescan of the whole folder
        ChangedPath::instance.changed_path(transfer_path.as_destination(), ChangedPath::Type::File, ChangedPath::Incident::Deleted);
        ChangedPath::instance.changed_path(transfer_path.as_destination(), ChangedPath::Type::File, ChangedPath::Incident::Created);
    } else {
        // else just send the FILE_INFO, to notify connect, that the filke is not read_only anymore
        ChangedPath::instance.changed_path(transfer_path.as_destination(), ChangedPath::Type::File, ChangedPath::Incident::Created);
    }
    log_info(transfers, "Transfer %s cleaned up", transfer_path.as_destination());

    return true;
}

bool Transfer::cleanup_remove(Path &path) {
    // Without this we would report LFN to connect in FILE_CHANGED event, which is not allowed.
    path.as_destination(); // Reset it to the "base"
    get_SFN_path(path.get_buffer());
    // Note: Order of removal is important. It is possible the partial can't be
    // removed (eg. because it's being shown as a preview, or being printed).
    // In such case we want to make sure _not_ to delete the (possibly failed)
    // backup.
    int remove_result = remove(path.as_partial());
    // Allow the partial-file not to exist any more (invalid state)
    bool success = (remove_result == 0 || errno == ENOENT);
    success = success && (remove(path.as_backup()) == 0) && (rmdir(path.as_destination()) == 0);

    if (success) {
        ChangedPath::instance.changed_path(path.as_destination(), ChangedPath::Type::File, ChangedPath::Incident::Deleted);
    } else {
        log_error(transfers, "Failed to remove aborted transfer %s", path.as_destination());
    }
    return success;
}

bool Transfer::store_transfer_index(const char *path) {
    auto index = unique_file_ptr(fopen(transfer_index, "a"));
    if (index.get() == nullptr) {
        return false;
    }

    return fprintf(index.get(), "%s\n", path) > 0; // Returns negative on error
    // auto-closed by unique_file_ptr
}
