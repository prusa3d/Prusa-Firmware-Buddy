#include <logging/log_dest_file.hpp>

#include <mutex>
#include <shared_mutex>
#include <cstring>

#include <common/freertos_shared_mutex.hpp>
#include <logging/log_dest_shared.hpp>
#include <libs/circularqueue.h>
#include <unique_file_ptr.hpp>
#include <async_job/async_job.hpp>

LOG_COMPONENT_REF(FileSystem);

namespace logging {

namespace file {

    struct BufferChunk {
        uint8_t size = 0;
        std::array<char, 31> data;
    };
    static_assert(sizeof(BufferChunk) == 32);

    struct Data {
        AtomicCircularQueue<BufferChunk, uint8_t, 16> buffer;
        AsyncJob write_job;
        unique_file_ptr file;

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

    const bool was_overflow = data->buffer.isFull();

    data->write_buffer();

    // If we fail writing, disable the logger
    if (ferror(data->file.get())) {
        _gd.unlock();
        file_log_disable();
        return;
    }

    // Log AFTER draining the buffer - we want this record to be stored in the file as well
    if (was_overflow) {
        log_warning(FileSystem, "Logging to file: buffer overflow");
    }
}

static void flush_chunk() {
    assert(data->wip_chunk.size <= data->wip_chunk.data.size());

    data->buffer.enqueue(data->wip_chunk);
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
    if (!d) {
        return false;
    }

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
