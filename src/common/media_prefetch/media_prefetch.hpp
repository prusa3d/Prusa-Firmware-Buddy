#pragma once

#include <cstring>
#include <cstddef>
#include <array>
#include <mutex>

#include <gcode/gcode_reader_any.hpp>
#include <async_job/async_job.hpp>
#include <freertos/mutex.hpp>

/// Handles loading data from a gcode file, keeps it in the ring buffer, and provides the gcode further down to the pipeline
class MediaPrefetchManager {

public:
    /// Size of the ring buffer
    static constexpr size_t buffer_size = 8192;

    enum class Status {
        /// All is well, command was fetched
        ok,

        /// Reached end of buffer, but it should eventually get more data - waiting for USB response, ...
        end_of_buffer,

        /// We've reached end of file, no more gcode will be coming
        end_of_file,

        /// The gcode is corrupted (CRC check did not match), the file needs to be fixed
        corruption,

        /// There is a problem with USB that needs to be resolved to continue fetching the data
        usb_error,

        /// We've hit an end of what is downloaded
        not_downloaded,
    };

    /// Returns whether the status \p s is an error that we should try recovering from somehow
    inline static bool is_error(Status s) {
        return (s != Status::end_of_buffer) && (s != Status::end_of_file) && (s != Status::ok);
    }

public:
    MediaPrefetchManager();
    ~MediaPrefetchManager();

public:
    struct ReadResult {
        /// Fetched gcode
        std::array<char, MAX_CMD_SIZE> gcode;

        /// Position for the fetched gcode - to resume the stream by repeating the gcode
        GCodeReaderPosition replay_pos;

        /// Position just AFTER the fetched gcode - to resume the stream from the right position
        GCodeReaderPosition resume_pos;

        /// If true, indicates that the gcode did not fit in the buffer and was cropped
        bool cropped;
    };

    struct StatusAndActive {
        // Status of the command.
        Status status;
        // Is some fetch running?
        //
        // If true, an error status might still "go away" on its own by this particular fetch.
        bool fetch_active;
    };

    /// Attempts to read one gcode command from the buffer.
    StatusAndActive read_command(ReadResult &result);

    /// Restarts the manager & starts reading gcode from the specified position
    void start(const char *filepath, const GCodeReaderPosition &position);

    /// Stops the machinery and resets the state
    void stop();

    /// Issues prefetch if the buffer is running empty.
    void issue_fetch();

    /// \returns if the buffer is empty, but we're still fetching.
    /// This is equivalent to the situation where \p read_command returns \p Status::end_of_buffer (but this is only a check, not reading anything)
    bool check_buffer_empty() const;

    enum class ReadyToStartPrintResult {
        /// There is not enoug data to start printing, we need to fetch more
        needs_fetching,

        /// The prefetch buffer is full, we can start printing
        ready,

        /// There was an error during the fetching
        error,
    };

    /// \returns whether the buffer is full or at EOF
    ReadyToStartPrintResult check_ready_to_start_print() const;

public:
    struct Metrics {
        /// Number of gcode commands currently in the buffer
        size_t commands_in_buffer;

        /// Estimated size of the stream
        size_t stream_size_estimate;

        /// How much full the fetch bufer is [0-100]. Only approximate, for statistics/reporting purposes.
        uint8_t buffer_occupancy_percent;

        /// Current status at the buffer end.
        /// The prefetch can possibly recover from the error states by itself.
        /// !!! To be used only for early warnings, for all other purposes, check result of read() !!!
        Status tail_status;
    };

    /// \returns various metrics regarding the media prefetch
    Metrics get_metrics() const;

    /// \returns filepath the prefetch is currently set up to
    inline auto filepath() const {
        std::lock_guard mutex_guard(mutex);
        return shared_state.filepath;
    }

private:
    /// Routine that is executed on a worker thread
    void fetch_routine(AsyncJobExecutionControl &control);

    /// Attempts to flush command from the command_buffer.
    /// \returns \p false if the execution of the fetch should stop
    bool fetch_flush_command(AsyncJobExecutionControl &control);

    /// Attempts to flush a command
    /// \returns \p false if the execution of the fetch should stop
    bool fetch_command(AsyncJobExecutionControl &control);

    void fetch_handle_error(AsyncJobExecutionControl &control, IGcodeReader::Result_t error);

private:
    /// \returns where the read contains at least \n bytes bytes
    bool can_read_entry_raw(size_t bytes) const;

    /// Reads one entry from the buffer.
    /// Follows the logic of \p write_entry writing.
    /// !!! Only to be done from the manager context
    /// !!! Assumes the \p mutex to be locked
    void read_entry_raw(void *target, size_t bytes);

    /// !!! Only to be done from the manager context
    /// !!! Assumes the \p mutex to be locked
    template <typename T>
    void read_entry(T &target) {
        static_assert(std::is_trivially_copyable_v<T>);
        read_entry_raw(&target, sizeof(T));
    }

    /// \returns whether there is enough space in the buffer to write \p bytes bytes.
    /// !!! Only to be done from the worker context
    bool can_write_entry_raw(size_t bytes) const;

    template <typename... Record>
    bool can_write_entry() const {
        return can_write_entry_raw((sizeof(Record) + ...));
    }

    /// Attempts to write the provided data on \p worker_state.write_tail
    /// !!! Only to be done from the worker context
    void write_entry_raw(const void *data, size_t bytes);

    template <typename T>
    void write_entry(const T &rec) {
        static_assert(std::is_trivially_copyable_v<T>);
        write_entry_raw(&rec, sizeof(T));
    }

private:
    /// Circular buffer, accessed from both the manager and worker asynchronously.
    /// Data in this buffer is a sequence RecordHeader + data, depending on the record type
    /// The buffer is split into three regions:
    /// - Data between \p read_head and \p read_tail are ready to be read byd the manager
    /// - Data between \p read_tail and \p write_tail are being built by the worker, but not yet handed over to the manager
    /// - The rest is free to be used by the worker
    std::array<uint8_t, buffer_size> buffer;

    /// Fields that can be accessed from both the manager and the worker. Accesses need to be behind \p mutex.
    struct {
        /// Read head - state of things just after the last gcode returned by \p read_command
        /// This is only written to by the manager, worker only reads it
        struct {
            /// First position in \p buffer that is safe to be read by the manager.
            /// Also means first position in buffer that is NOT safe modifiable by the worker.
            size_t buffer_pos = 0;

        } read_head;

        /// Read tail = write head - end of the region that is ready to be read by the manager, start of the worker domain
        /// This is only read from by the manager, only the worker can write to this (except for stop state)
        struct {
            /// Gcode position where se should resume to fill more data in the buffer
            GCodeReaderPosition gcode_pos;

            /// First position in \p buffer that is NOT to be read my the manager. This is there the worker domain starts.
            /// Also means first position the worker can safely write to.
            size_t buffer_pos = 0;

            /// Reason why there are no more data to be read in the buffer.
            /// This is the status to be returned when manager reads all the data from the buffer.
            /// This should NEVER be \p Status::ok
            Status status = Status::end_of_file;

        } read_tail;

        size_t stream_size_estimate = 0;

        /// If set to true, indicates that the worker should completely reset its state and try start fetching from \p read_tail
        bool worker_reset_pending = true;

        /// If set to false, means that no fetching is actually requested and the prefetch should only try to open the file
        bool fetch_requested = false;

        /// Number of commands stored in the buffer
        size_t commands_in_buffer = 0;

        /// A way to pass the filename to the worker in a managed way
        std::array<char, FILE_PATH_BUFFER_LEN> filepath = { '\0' };

    } shared_state;

    /// Fields only to be accessed from the manager, never by the worker
    struct {
        /// Read head - state of things just after the last gcode returned by \p read_command
        struct {
            /// Gcode position after the last gcode returned by \p read_command
            /// This is the position to resume from if we would discard the entire prefetch buffer
            GCodeReaderPosition gcode_pos;

        } read_head;

    } manager_state;

    /// Fields that are managed by the worker. The manager should never touch these.
    struct {
        AnyGcodeFormatReader gcode_reader;

        /// Current position in gcode_reader
        uint32_t gcode_reader_pos = 0;

        /// Buffer for the command we are currently reading
        std::array<char, MAX_CMD_SIZE> command_buffer_data = { '\0' };

        struct {
            /// Write position in the command_buffer
            size_t write_pos = 0;

            /// Indicates whether the command buffer is ready to be flushed in the main buffer
            bool flush_pending = false;

            /// Indicates that we should skip the rest of the curent line - probably because we're reading a comment
            bool skip_rest_of_line = false;

            /// If set, denotes that the gcode did not fit into the buffer and was cropped
            bool cropped = false;

        } command_buffer;

        struct {
            /// First position in the (shared) buffer that is available for writing
            size_t buffer_pos = 0;

            GCodeReaderPosition gcode_pos;

        } write_tail;

        struct {
            /// Copy from \p shared_state for local usage
            size_t buffer_pos = 0;

        } read_head;

    } worker_state;

    /// Mutex that protects \p shared_state
    mutable freertos::Mutex mutex;

    /// Job representing the pending \p fetch_routine call
    AsyncJob worker_job;
};
