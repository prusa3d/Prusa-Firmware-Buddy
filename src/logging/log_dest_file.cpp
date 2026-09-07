#include <logging/log_dest_file.hpp>

#include <mutex>
#include <shared_mutex>
#include <cstring>

#include <common/freertos_shared_mutex.hpp>
#include <logging/log_dest_shared.hpp>
#include <utils/atomic_circular_queue.hpp>
#include <unique_file_ptr.hpp>
#include <async_job/async_job.hpp>
#include <sys/unistd.h>
#include <freertos/timing.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(FileSystem);

namespace logging {

namespace file {

    struct BufferChunk {
        uint8_t size = 0;
        std::array<char, 31> data;
    };
    static_assert(sizeof(BufferChunk) == 32);

    struct Data {
        AtomicCircularQueue<BufferChunk, uint8_t, 32> buffer;
        AsyncJob write_job;
        unique_file_ptr file;
        uint32_t last_flush_time_ms = 0;
        uint8_t file_errors = 0;

        BufferChunk wip_chunk;

        void write_buffer() {
            while (!buffer.isEmpty()) {
                BufferChunk chunk = buffer.dequeue();
                fwrite(chunk.data.data(), 1, chunk.size, file.get());
            }
        }

        ~Data() {
            if (file) {
                write_buffer();
            }
        }
    };

    std::atomic_bool is_enabled = false;

    /// Support structure for the logger. Gets dynamically allocated only when the logger is active (it almost never is).
    std::unique_ptr<Data> data = nullptr;

    freertos::SharedMutex mutex(2);

} // namespace file

using namespace file;

static void file_log_write(AsyncJobExecutionControl &) {
    std::shared_lock _gd(mutex);

    if (!data) {
        return;
    }

    FILE *file = data->file.get();
    const bool was_overflow = data->buffer.isFull();

    data->write_buffer();

    if (was_overflow) {
        // Write a newline on overflow - it was likely chopped
        // You know what, write two to visually separate the sections
        fwrite("\n\n", 1, 2, file);
    }

    uint32_t now = freertos::millis();
    constexpr uint32_t auto_flush_period_ms = 60'000;
    if (now - data->last_flush_time_ms > auto_flush_period_ms) {
        if (fflush(file) == 0 && fsync(fileno(file)) == 0) {
            // flush and sync was successfull
            data->last_flush_time_ms = now;
        }
    }

    if (ferror(file)) {
        // message cannot be logged, sus
        clearerr(file);
        data->file_errors++;
        constexpr uint8_t max_tries = 10;
        if (data->file_errors >= max_tries) {
            // too much errors, flash drive was probably removed
            _gd.unlock();
            file_log_disable();
        }
        return;
    }
    if (data->file_errors != 0) {
        // logging works again
        log_warning(FileSystem, "Last %d messages cannot be logged", data->file_errors);
        data->file_errors = 0;
    }

    // Log AFTER draining the buffer - we want this record to be stored in the file as well
    if (was_overflow) {
        log_warning(FileSystem, "Logging to file: buffer overflow");
    }
}

static void flush_chunk() {
    debug_assert(data->wip_chunk.size <= data->wip_chunk.data.size());

    (void)data->buffer.enqueue(data->wip_chunk);
    data->wip_chunk.size = 0;
}

static void log_char(char ch) {
    data->wip_chunk.data[data->wip_chunk.size++] = ch;

    if (data->wip_chunk.size == data->wip_chunk.data.size()) {
        flush_chunk();
    }
}

void file_log_event(FormattedEvent *event) {
    // Early check to prevent mutex locking when it's 99% of the time unnecessary
    if (!is_enabled.load()) {
        return;
    }

    std::shared_lock guard(mutex);

    // Check again - might have changed before we acquired the mutex
    if (!data) {
        return;
    }

    // We're in the logging task – we want to be as little blocking as possible.
    // Use atomic queue and do the actual writes in a low priority async thread.
    log_format_simple(
        event, [](char ch, void *) { log_char(ch); }, nullptr);
    log_char('\n');
    flush_chunk();

    // If the write job is already running, it might be exitting right now and might miss the last additions -> enqueue another
    if (data->write_job.state() != AsyncJobBase::State::queued) {
        data->write_job.issue(file_log_write);
    }
}

bool file_log_enable(const char *filepath) {
    // Do outside of mutex to minimize locking
    auto d = std::make_unique<Data>();
    d->file.reset(fopen(filepath, "a"));
    if (!d->file) {
        return false;
    }

    {
        std::scoped_lock guard(mutex);
        data = std::move(d);
        is_enabled = true;
    }

    log_info(FileSystem, "Started logging to file: %s", filepath);

    return true;
}

void file_log_disable() {
    log_info(FileSystem, "Stopped logging to file");

    std::scoped_lock guard(mutex);
    is_enabled = false;
    data.reset();
}

bool file_log_is_enabled() {
    return is_enabled.load();
}

} // namespace logging
