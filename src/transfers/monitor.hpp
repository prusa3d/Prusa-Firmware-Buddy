#pragma once

#include <freertos/mutex.hpp>
// Why is the FILE_PATH_BUFFER_LEN in gui?
#include <gui/file_list_defs.h>

#include "partial_file.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <mutex>

namespace transfers {

using TeamId = uint64_t;
using TransferId = uint32_t;
using Timestamp = uint32_t;

/// The monitor of transfers.
///
/// This holds info about the currently running transfer of a file (from Slicer
/// / Link / Connect), if any. It also holds some (significantly reduced) info
/// about some past transfer(s) ‒ mostly, just their outcome.
///
/// The purpose is to share the information between threads and reserve the
/// single active transfer, in a thread-safe way.
///
/// It is meant to operate in a single instance (which is bundled inside) in
/// normal operation. It is however not a true singleton ‒ it is possible to
/// create an instance independent of the global one, which might be useful for
/// example in testing.
///
/// # Expected usage
///
/// The thread that desires to perform a transfer tries to claim the slot by
/// the allocate method. These may fail when the slot is already taken. If
/// successful, caller is supposed to hold onto the returned object for the
/// time of running the transfer and push all updates to it through that
/// object. Once the object is destroyed, the transfer is considered done and
/// the slot is returned.
///
/// Nevertheless, under normal circumstances, the thread shall call the done
/// method on the Slot to record how it finished.
///
/// Any observing thread may, at any time, call the status method to obtain
/// info about currently running transfer. This can return nullopt in case no
/// transfer is running.
///
/// # Lifetimes and locking
///
/// The user of this should be aware of few implementation details here.
///
/// * While the Slot is expected to be a long-living object (held for the whole
///   duration of the transfer), this is not the case for the Status object.
///   That one holds an exclusive lock on the monitor, so it can guarantee the
///   values won't change during its lifetime. But that also means it blocks
///   other threads from doing anything with the monitor at the same time. As a
///   result, the user shall not keep a Status alive for extended periods of
///   time and shall drop it (and re-acquire as needed) across any network
///   communications or other slow operations.
/// * As a consequence, the values between two Statuses may be different. It is
///   recommended to check that the transfer ID stays the same.
/// * As another consequence, calling the allocate method or dropping the Slot
///   while the same threads holds a Status will deadlock. It is, however,
///   possible (and expected) to get a Status after the thread acquired the Slot
///   and it is guaranteed that the Status will be for the same transfer as the
///   Status.
/// * Similarly, calling methods on the Slot while holding a Status will deadlock.
/// * Neither Status nor Slot are allowed to outlive the Monitor they came from
///   (not an issue with the global instance, of course).
/// * Any pointers or references acquired from a Status is valid only for as
///   long as the Status itself is alive.
/// * The outcome and current_id are independent of the main locking and
///   never deadlock.
class Monitor {
private:
    using Mutex = freertos::Mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    /// What kind of transfer it is.
    enum class Type {
        /// Upload to the printer.
        ///
        /// The printer is the Web server, the client uploads there (either
        /// with POST or PUT, we currently support both old and new way).
        ///
        /// This is usually the Link web page or Slicer.
        Link,

        /// Transfer from the Connect server.
        ///
        /// The printer is downloading from the Connect server.
        Connect,
    };

    /// An outcome of a transfer.
    enum class Outcome {
        /// It finished successfuly.
        Finished,

        /// The Connect (or some other system) asked us to bail out.
        Stopped,

        ErrorNetwork,
        ErrorStorage,
        ErrorOther,
    };

    /// A Status of a running transfer.
    class Status {
    private:
        friend class Monitor;
        Lock lock;
        Status(Lock &&lock)
            : lock(std::move(lock)) {}

    public:
        Status(Status &&other) = default;
        Status &operator=(Status &&other) = default;
        Status(const Status &other) = delete;
        Status &operator=(const Status &other) = delete;
        Type type;
        TransferId id;

        /// When the transfer started.
        ///
        /// In our own internal timestamp ‒ not necessarily anchored to
        /// anything and may wrap around!
        ///
        /// In seconds.
        Timestamp start;

        /// The expected size.
        size_t expected;

        /// Only valid for Link, otherwise false
        bool print_after_upload;

        /// How much got already transferred.
        PartialFile::State download_progress;

        /// True if there is an ongoing issue with the download
        bool download_has_issue;

        /// The path where the transfer will be stored (once it's done).
        ///
        /// Note that usually the file doesn't yet exist, the data is being
        /// saved into some kind of temp file and then moved.
        ///
        /// Also this could be nullptr if uploading by POST in Link,
        /// because in that case the path is not known from the beginning of
        /// the upload.
        const char *destination;

        double progress_estimate() const;
        uint32_t time_remaining_estimate() const;
        uint32_t time_transferring() const;
    };

    /// An allocated transfer slot.
    ///
    /// Keep alive while the transfer is running.
    class Slot {
    private:
        friend class Monitor;
        Monitor &owner;
        Slot(Monitor &owner);
        // Set to false when moved from.
        bool live;

    public:
        Slot(Slot &&other);
        /// Exists only to satisfy compiler in the optional<Slot>::operator=.
        /// But not to be called in reality (because there shall _not_ be two
        /// living slots at the same time anyway), the left side of that
        /// optional shall be nullopt!
        Slot &operator=(Slot &&other);
        Slot(const Slot &other) = delete;
        Slot &operator=(const Slot &other) = delete;
        ~Slot();

        void update_expected_size(size_t expected);

        /// Update the progress report.
        void progress(PartialFile::State download_state, bool has_issue);

        /// Update the progress report.
        ///
        /// This is the increment, not the accumulated total.
        void progress(size_t add_bytes);

        // Damn. We have to start over from the beginning...
        void reset_progress();

        /// The path where the transfer will be stored (once it's done).
        const char *destination();

        bool is_stopped();
        /// The transfer is done.
        ///
        /// This reports the outcome of this transfer. The Slot shall not be
        /// used more and shall be dropped consequently.
        ///
        /// If not called before dropping Error is automatically assigned.
        void done(Outcome reason);

        const char *filepath();

        TransferId id() const;
    };

    friend class Slot;

private:
    // Mutex for the "active transfer" information.
    mutable Mutex main_mutex;
    // Mutex for the history of outcomes.
    mutable Mutex history_mutex;
    // Changes to these _are_ protected by a mutex, but it is possible to
    // _read_ them without locking.
    //
    // A bit of care needs to be employed when dealing with them, to avoid
    // races, etc.
    std::atomic<bool> transfer_active = false;
    std::atomic<TransferId> current_id = 0;

    // Transfer related
    bool used = false;
    bool stopped = false;
    Type type;
    Timestamp start;
    size_t expected;
    PartialFile::State download_progress;
    bool download_has_issue;
    bool print_after_upload { false };

    char destination_path[FILE_PATH_BUFFER_LEN + FILE_NAME_MAX_LEN];

    // History related
    static constexpr size_t HISTORY_MAX_LEN = 2;
    size_t history_len = 0;
    TransferId history_latest;
    std::array<Outcome, HISTORY_MAX_LEN> history;

public:
    /// Signals to stop a transfer if running.
    ///
    /// The code responsible for doing the transfer should
    /// periodically check the stop signal by calling Slot::is_stopped().
    ///
    /// Returns true, if transfer is running, therefore we can stop it,
    /// false otherwise.
    bool signal_stop();

    std::optional<Slot> allocate(Type type, const char *dest, size_t expected_size, bool print_after_upload = false, std::optional<TransferId> override_id = std::nullopt);

    /// Request the status of currently running transfer.
    ///
    /// In case no transfer is running, this usually returns nullopt.
    ///
    /// If the allow_stale is set to true, a status for already terminated
    /// transfer may be returned (if there's one).
    ///
    /// The use case is, if the caller needs to release the Status across a
    /// network communication or something like that, the first status call
    /// would be done without allowing stale, while the follow-up would enable
    /// it. In case the transfer terminates in between, it would still get the
    /// rest of the data.
    ///
    /// Note that even then it still can happen that the new Status is for a
    /// newer transfer than the original one. In such case, the caller is
    /// advised to compare the original transfer ID with the new one and decide
    /// what to do if they are different.
    std::optional<Status> status(bool allow_stale = false) const;
    /// The outcome of a past transfer.
    ///
    /// It is possible to request an outcome of already complete transfer by
    /// its ID. We don't store the whole history, so if many transfers happened
    /// in between, it might no longer be available.
    ///
    /// This returns nullopt if the outcome is not known (an invalid ID, still
    /// running, or already lost to history). It returns Dropped in case we
    /// have the result stored, but wasn't properly set.
    std::optional<Outcome> outcome(TransferId id) const;

    /// The current transfer ID.
    ///
    /// This doesn't lock and can be used to quickly check if anything
    /// important changed without really going through the whole Status thing.
    std::optional<TransferId> id() const;

    /// The global instance.
    static Monitor instance;
};

const char *to_str(Monitor::Type type);

} // namespace transfers
