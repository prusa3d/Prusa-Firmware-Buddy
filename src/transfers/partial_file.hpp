/// @file
#pragma once

#include <memory>
#include <variant>
#include <optional>
#include <cstdint>
#include <cstddef>

#include <freertos/mutex.hpp>

namespace transfers {

/// A partial file tracks which parts of a file have been written, supporting
/// resumable downloads. While being written, it can be read by standard means
/// (fread etc.); get_valid_head()/get_valid_tail() tell which parts are valid
/// for reading.
///
/// To keep the implementation simple, at most 2 valid parts are tracked:
/// - the "head", starting at offset 0,
/// - the "tail", starting somewhere in the middle of the file and gradually
///   growing to its end.
///
/// Every write must extend the head or the tail; creating a third valid part
/// is not allowed. When the head and the tail meet, they are merged into one.
///
/// Concrete implementations handle the actual I/O — either direct USB sector
/// writes (UsbPartialFile) or standard POSIX fwrite (PosixPartialFile).
///
/// Thread safety: getting the state (and related methods) is thread safe and
/// can happen concurrently to writes from another thread.
class PartialFile {
public:
    /// USB writes are sector-aligned; download offsets are rounded to this
    /// boundary so resumed downloads land on sector starts. POSIX/littlefs
    /// writes have no alignment requirement, but use the same constant to keep
    /// callers (transfer.cpp) impl-agnostic.
    static constexpr size_t SECTOR_SIZE = 512;

    struct ValidPart {
        size_t start;
        size_t end; // exclusive

        void merge(const ValidPart &other) {
            // this:  oooox
            // other:     oooox
            if (other.start <= end && other.end > end) {
                // extend to the right
                end = other.end;
            }
            // this:        oooox
            // other:   oooox
            // other:      ox
            if (other.start < start && other.end >= start) {
                // extend to the left
                start = other.start;
            }
        }
    };

    struct State {
        std::optional<ValidPart> valid_head {};
        std::optional<ValidPart> valid_tail {};
        size_t total_size = 0;

        size_t get_valid_size() const {
            size_t bytes_head = valid_head.has_value() ? valid_head->end - valid_head->start : 0;
            size_t bytes_tail = valid_tail.has_value() ? valid_tail->end - valid_tail->start : 0;
            size_t bytes_overlap = 0;
            if (valid_head.has_value() && valid_tail.has_value() && valid_head->end > valid_tail->start) {
                bytes_overlap = valid_head->end - valid_tail->start;
            }
            return bytes_head + bytes_tail - bytes_overlap;
        }

        bool fully_valid() const {
            return get_valid_size() == total_size;
        }

        size_t get_percent_valid() const {
            // float on purpose: size * 100 can overflow size_t
            return total_size ? static_cast<size_t>(static_cast<float>(get_valid_size()) * 100.0f / total_size) : 0;
        }

        void extend_head(size_t bytes) {
            if (valid_head.has_value()) {
                valid_head->end += bytes;
            } else {
                valid_head = { 0, bytes };
            }
        }
    };

    using Ptr = std::shared_ptr<PartialFile>;
    using Result = std::variant<const char *, Ptr>;

    /// USB sector buffers passed through `get_current_buffer` come as
    /// `BufferAndSizes`. POSIX returns the same shape (over a small RAM
    /// staging buffer) so callers don't have to special-case.
    struct WouldBlock {};
    struct WriteError {};
    struct OutOfRange {};
    struct BufferAndSizes {
        uint8_t *buffer;
        /// Already written into the buffer this far; new data goes to buffer + offset.
        size_t offset;
        /// Usable size of the buffer; may differ from call to call. Does not
        /// account for the end of file, that's up to the caller.
        size_t size;
    };
    using BufferPeek = std::variant<BufferAndSizes, WouldBlock, WriteError, OutOfRange>;

    using WrittenCallback = void(void *);

    virtual ~PartialFile() = default;

    /// Create a new partial file at path with the given size.
    /// Selects the implementation based on the path's storage type.
    static Result create(const char *path, size_t size);

    /// Open an existing partial file. `state` is the resume state from a
    /// previous transfer; USB overwrites `state.total_size` from the on-disk
    /// file, POSIX preserves it (no preallocation).
    ///
    /// Always allows another reader to hold the file (the download screen or an
    /// active print): on FAT the pin handle is opened read-only so it coexists
    /// with that reader (FF_FS_LOCK makes a write-open exclusive, BFW-4542),
    /// which is fine because USB writes via raw sectors. littlefs writes
    /// through the handle, so it opens read-write (it has no exclusive-write
    /// lock).
    static Result open(const char *path, State state);

    /// Whether the implementation supports efficient random writes (seek+write).
    /// USB/FAT does (preallocated contiguous file). POSIX/littlefs doesn't
    /// (no sparse files, copy-on-write blocks). Used to select download order.
    virtual bool supports_random_write() const = 0;

    /// Seek is only allowed to a write-buffer (USB: sector) boundary. A
    /// partially filled buffer is not carried over: USB discards it, POSIX
    /// flushes it first.
    virtual bool seek(size_t offset) = 0;
    virtual size_t tell() = 0;

    /// Write data at the current offset.
    virtual bool write(const uint8_t *data, size_t size) = 0;

    /// Commit everything written so far to the medium and extend the valid
    /// parts over it. Used to complete the tail of a download.
    virtual bool sync() = 0;

    /// "Close" the file while still preserving the state and size. No further
    /// writes will succeed and the lock on the file is released.
    virtual void release_file() = 0;

    /// Expose the current write buffer, allocating a new one as needed; the
    /// caller writes at buffer + offset. Consecutive writes fill the buffer
    /// gradually and it is flushed to the medium when full (see
    /// advance_written()). With blocking_wait, waits for a buffer to become
    /// available instead of returning WouldBlock.
    virtual BufferPeek get_current_buffer(bool blocking_wait) = 0;

    /// Advance the write position; flushes the buffer to the medium once
    /// fully written.
    virtual bool advance_written(size_t by) = 0;

    State get_state() const;
    size_t final_size() const { return get_state().total_size; }
    std::optional<ValidPart> get_valid_head() const { return get_state().valid_head; }
    std::optional<ValidPart> get_valid_tail() const { return get_state().valid_tail; }
    /// Check if the file has valid data in the [0, bytes) range.
    bool has_valid_head(size_t bytes) const;
    /// Check if the file has valid data in the [total_size - bytes, total_size) range.
    bool has_valid_tail(size_t bytes) const;

    /// Set a callback invoked whenever a block is written or fails to be
    /// written. Can be set to null.
    void set_written_callback(WrittenCallback *cb, void *arg) {
        written_callback = cb;
        written_callback_arg = arg;
    }

protected:
    explicit PartialFile(State initial)
        : state(initial) {}

    /// Log the overall progress when the percentage changes.
    ///
    /// Call from the task performing the writes. Deliberately not part of
    /// extend_valid_part(): that can run in the USB MSC completion callback
    /// on the realtime worker task, where blocking on the log task would
    /// stall all USB I/O.
    void log_progress();

    mutable freertos::Mutex state_mutex;
    State state;
    int last_progress_percent = -1;

    WrittenCallback *written_callback = nullptr;
    void *written_callback_arg = nullptr;

    /// Extend head/tail to include `new_part`. Locks state_mutex. May run in
    /// the USB MSC completion context - keep it free of blocking calls.
    ///
    /// Invariant: callers only extend over data already durable on the medium
    /// (USB: from the write-completion callback, POSIX: after fsync), so
    /// persisting the state (transfer backup) is always safe.
    void extend_valid_part(ValidPart new_part);

    /// Invoke the registered write callback, if any.
    void notify_written();
};

} // namespace transfers
