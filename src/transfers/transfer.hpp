#pragma once
#include <stdint.h>
#include <sys/stat.h>

#include <common/mutable_path.hpp>
#include <common/unique_file_ptr.hpp>
// Why is the FILE_PATH_BUFFER_LEN in gui?
#include <gui/file_list_defs.h>

#include "monitor.hpp"
#include "partial_file.hpp"
#include "download.hpp"
#include "transfer_file_check.hpp"

namespace transfers {

inline constexpr size_t MAX_RETRIES = 5;

struct NoTransferSlot {};

/// Represents a transfer of a file from Connect to the printer.
///
/// It is initiated via Transfer::begin() with a given download request and a destination path.
/// A transfer is uniquely identified by its destination path - /usb/file.gcode.
/// This class is responsible for managing the download (retrying on errors, reinitiating the download from
///   different offsets in order to download interesting parts of the file first, etc.).
/// When a transfer is initiated with destination "/usb/file.gcode", the transfer is represented in the filesystem as follows:
///    /usb/file.gcode/ (directory)
///    /usb/file.gcode/p (file; preallocated to the final sizes and stores the raw downloaded data)
///    /usb/file.gcode/d (file; stores progress of the transfer and info for its recovery)
/// When the transfer is fully downloaded, the /usb/file.gcode/download is removed.
/// Later, when the printer is in idle state, the /usb/file.gcode/p is moved to its final destination /usb/file.gcode.
class Transfer {
public:
    enum class State {
        Downloading,
        Retrying,
        Finished,
        Failed,
    };

private:
    //
    // Download Order Helpers
    //

    /// Action returned by DownloadOrder::step().
    enum class Action {
        Continue,
        RangeJump,
        Finished,
    };

    /// Downloads the beginning and ending of the file first (so it can be scanned by GcodeInfo) and print can be started.
    class PlainGcodeDownloadOrder {
        // TODO: Use values based on GCodeInfo
        static constexpr size_t HeadSize = 400 * 1024;
        static constexpr size_t TailSize = 50000;

        enum class State {
            DownloadingTail,
            DownloadingBody,
        };

        State state = State::DownloadingTail;

    public:
        static constexpr size_t MinimalFileSize = 512 * 1024;
        static_assert(MinimalFileSize > HeadSize + TailSize + 1);

        PlainGcodeDownloadOrder(const PartialFile &partial_file);

        Action step(const PartialFile &file);

        /// Returns offset from where to continue downloading the file
        size_t get_next_offset(const PartialFile &file) const;
    };

    class GenericFileDownloadOrder {
    public:
        Action step(const PartialFile &);

        /// Returns offset from where to continue downloading the file
        size_t get_next_offset(const PartialFile &file) const;
    };

    using DownloadOrder = std::variant<GenericFileDownloadOrder, PlainGcodeDownloadOrder>;

public:
    //
    // File Path Helpers
    //

    /// Helps figuring out the paths of different files of a transfer.
    ///
    /// Warning: Any path you get from this instance is valid only until the next call to any of its methods!
    class Path {
        MutablePath path;
        bool is_destination = true;

    public:
        Path() = default;

        /// Get an instance given the base "destination" path (e.g. "/usb/file.gcode").
        Path(const char *destination_path)
            : path(destination_path)
            , is_destination(true) {
        }

        /// Get the base "destination" path (e.g. "/usb/file.gcode").
        const char *as_destination() {
            if (!is_destination) {
                path.pop();
            }
            is_destination = true;
            return path.get();
        }

        /// Get the path to the download file (e.g. "/usb/file.gcode/download").
        const char *as_backup() {
            if (!is_destination) {
                path.pop();
            }
            path.push(backup_filename);
            is_destination = false;
            return path.get();
        }

        /// Get the path to the partial file (e.g. "/usb/file.gcode/partial").
        const char *as_partial() {
            if (!is_destination) {
                path.pop();
            }
            path.push(partial_filename);
            is_destination = false;
            return path.get();
        }

        void reset_flags() {
            is_destination = true;
        }

        char *get_buffer() {
            return path.get_buffer();
        }

        static constexpr size_t maximum_length() {
            return MutablePath::maximum_length();
        }
    };

private:
    static bool cleanup_finalize(Path &transfer_path);
    static bool cleanup_remove(Path &transfer_path);

    //
    // Transfer Serialization
    //
public:
    class RestoredTransfer {
    private:
        static constexpr size_t max_size = 512; // read no more than this from backup file to avoid out of memory on corrupted filesystem

        FILE *file;

        PartialFile::State partial_file_state;

        std::unique_ptr<char[]> data_buffer;
        size_t data_buffer_size;

        const void *get_data_ptr(size_t offset, size_t size);

    public:
        RestoredTransfer(FILE *file, PartialFile::State partial_file_state, TransferId id);

        /// Returns the PartialFile::State that was stored in the backup file.
        PartialFile::State get_partial_file_state() { return partial_file_state; }

        /// Returns the Download::Request that was stored in the backup file.
        /// Warning: The returned request is valid only for the lifetime of the RestoredTransfer instance.
        std::optional<Download::Request> get_download_request();

        TransferId id;
    };

    /// Store given Download::Request and PartialFile::State to the given file.
    /// The file's content is overriden (in case the file exists) and ftell() on return is undefined.
    static bool make_backup(FILE *file, const Download::Request &request, const PartialFile::State &state, const Monitor::Slot &slot);

    /// Updates a backup file created with make_backup() with the given PartialFile::State.
    static bool update_backup(FILE *file, const PartialFile::State &state);

    /// Allows restoration of the Download::Request and PartialFile::State from the given backup file
    /// through the RestoredTransfer interface.
    static std::optional<RestoredTransfer> restore(FILE *file);

    struct Error {
        /// Optional error message
        const char *msg = nullptr;
    };
    struct Complete {};

    /// Tries to read the backup file for a running transfer for given path.
    ///
    /// This is the "base" path (eg. not with /backup at the end). Returns:
    ///
    /// * Complete if the file is completely transferred and available
    /// * Error if something is broken but the file is not available (probably won't be recoverable?)
    /// * A PartialFile::State with information about that file.
    static std::variant<Error, Complete, PartialFile::State> load_state(const char *path);

    /// If the path is a valid transfer (checked by "is_valid_transfer" above) it will return stat
    /// of the partial file, otherwise nullopt is returned.
    static std::optional<struct stat> get_transfer_partial_file_stat(MutablePath &destination_path);

    //
    // Internal
    //

    static constexpr uint32_t BackupUpdateIntervalMs = 10000;

private:
    /// Slot of the transfer
    Monitor::Slot slot;

public:
    /// Active download (if any)
    std::optional<Download> download;

private:
    /// Path helper to be able to retrieve paths to different files of the transfer.
    Path path;

    /// Helper object defining in which order to download the file.
    std::optional<DownloadOrder> order;

    /// Timestamp of the last connection error (used to throttle retries)
    std::optional<uint32_t> last_connection_error_ms;

    /// Restart of download requested by seeking / jumping in the file.
    ///
    /// Prevents from going to "orange" error when it's just seeking.
    bool restart_requested_by_jump = false;

    /// Force an update at reaching some initial size.
    ///
    /// In an effort to speed up print start / preview. The size is just a guess.
    bool initial_part_done = false;

    /// Timestamp of the last time the backup file was updated (used to throttle the updates)
    std::optional<uint32_t> last_backup_update_ms;

    /// Allow at most this many retries for network errors.
    size_t retries_left = MAX_RETRIES;
    /// Reset download retries when this changes... when we make some progres.
    size_t last_downloaded_size = 0;

    /// Current state of the transfer
    State state;

    /// The partial file where the downloaded data is stored
    PartialFile::Ptr partial_file;

    /// Make sure we notify only once.
    ///
    /// The notification to connect would be OK multiple times (nothing would
    /// break), but we also show the one-click-print as part of the
    /// notification which could be annoying to the user.
    bool already_notified = false;

    /// Is the file printable (as in, is it gcode or bgcode?).
    ///
    /// We notify about the existence of printable files while downloading. We
    /// are much more conservative for transfers that aren't printables (eg. a
    /// bbf) because they don't work until they are properly in place and fully
    /// complete. For these, we notify at the very end and we actually put them
    /// into place before that.
    bool is_printable = false;

    Transfer(Monitor::Slot &&slot, PartialFile::Ptr partial_file);

    /// Initiates a new download() request based on current state. Returns true on succes.
    /// TODO: We have to better handle errors here (distinguish between recoverable and non-recoverable ones)
    bool restart_download();

    void done(State state, Monitor::Outcome outcome);

    /// For some reasons we might not have a download order from the very beginning.
    /// This creates it if needed.
    void init_download_order_if_needed();

    /// Enqueue a notification about the file being successfuly created.
    void notify_created();

    /// Enqueue a full notification about the file being available.
    ///
    /// It is slighly duplicated with notify_created, but that one is just
    /// FILE_CHANGED, this one is FILE_INFO with full details (eg. previews).
    void notify_success();

    enum class IndexIter {
        Ok,
        Skip,
        Eof,
        IndividualError,
        FatalError,
    };

    static IndexIter next_in_index(unique_file_ptr &index, Path &out);

    // We have a file with list of transfers that are still in the form of directory.
    //
    // This is one line per file, LFN (at least the last segment).
    // These are candidates - that is, transfer listed here can be already moved
    // into place, not exist, etc. It just helps us not to search the whole USB.
    static constexpr const char *transfer_index = "/usb/.prusa_transfers.txt";

    static bool store_transfer_index(const char *path);

    //
    // Main Transfer Interface
    //

public:
    Transfer(Transfer &&other) = default;
    Transfer(const Transfer &other) = delete;
    Transfer &operator=(Transfer &&other) = default;
    Transfer &operator=(const Transfer &other) = delete;

    using BeginResult = std::variant<Transfer, NoTransferSlot, AlreadyExists, Storage>;
    /// Begin a new transfer.
    static BeginResult begin(const char *destination_path, const Download::Request &request);

    using RecoverResult = std::variant<Transfer, NoTransferSlot, Storage>;
    /// Given a destination path, restore the transfer (based on the <dest_path>/download file).
    static RecoverResult recover(const char *dest_path);

    State step(bool is_printing);

    enum class RecoverySearchResult {
        WaitingForUSB,
        NothingToRecover,
        Success,
    };
    /// Looks for transfers that can be recovered with the `path` directory.
    /// If any is found, it returns RecoverySearchResult::Success and the `path` points to the destination_path of the transfer.
    static RecoverySearchResult search_transfer_for_recovery(Path &path);

    /// Goes through all the transfers in the `root_path` directory.
    /// If a transfer is fully downloaded (is missing the /download file), it is "cleaned up" (the partial file is moved to the destination file location).
    ///
    /// If a download file is present, but empty, it failed and it is removed completely.
    ///
    /// After fully successful cleanup, the index file is removed.
    static bool cleanup_transfers();

    /// Counts retries and either aborts or schedules a next attempt.
    void recoverable_failure(bool is_printing);

    /// Updates the backup file if needed
    /// Can be forced (in case we want to make sure we backup the current state).
    void update_backup(bool force = false);
};

} // namespace transfers
