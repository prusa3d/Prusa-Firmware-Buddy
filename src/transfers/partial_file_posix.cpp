#include "partial_file_posix.hpp"

#include <common/bsod.h>
#include <logging/log.hpp>
#include <cstdio>
#include <unistd.h>

LOG_COMPONENT_REF(transfers);

namespace transfers {

PosixPartialFile::PosixPartialFile(unique_file_ptr file, State state)
    : PartialFile(state)
    , file(std::move(file)) {}

PosixPartialFile::~PosixPartialFile() {
    // Flush any buffered data and sync to ensure littlefs commits
    // the file size to the directory entry before closing.
    // A failed flush resets `file`; fflush(NULL) would flush all open streams.
    if (file && flush_write_buf()) {
        if (fflush(file.get()) != 0) {
            log_error(transfers, "fflush: errno %d", errno);
        }
    }

    file.reset();
}

PartialFile::Result PosixPartialFile::create(const char * /*path*/, unique_file_ptr file, size_t size) {
    State initial_state { .total_size = size };

    return std::make_shared<PosixPartialFile>(std::move(file), initial_state);
}

PartialFile::Result PosixPartialFile::open(const char * /*path*/, unique_file_ptr file, State state) {
    return std::make_shared<PosixPartialFile>(std::move(file), state);
}

bool PosixPartialFile::seek(size_t offset) {
    if (!file) {
        return false;
    }
    if (!flush_write_buf() || !sync_flushed()) {
        return false;
    }
    if (fseek(file.get(), offset, SEEK_SET) != 0) {
        return false;
    }
    current_offset = offset;
    synced_offset = offset;
    return true;
}

bool PosixPartialFile::write(const uint8_t *data, size_t size) {
    if (!file) {
        return false;
    }
    if (current_offset + size > state.total_size) {
        log_error(transfers, "PosixPartialFile: write past end (%u + %u > %u)",
            (unsigned)current_offset, (unsigned)size, (unsigned)state.total_size);
        return false;
    }

    // Keep ordering in case the caller interleaves this with the buffer API
    if (!flush_write_buf()) {
        return false;
    }

    debug_assert(ftell(file.get()) == static_cast<long>(current_offset));
    size_t written = fwrite(data, 1, size, file.get());
    if (written != size) {
        log_error(transfers, "PosixPartialFile: write failed at offset %u, wanted %u, got %u",
            (unsigned)current_offset, (unsigned)size, (unsigned)written);
        // the stream position no longer matches current_offset, prevent further use
        file.reset();
        return false;
    }

    current_offset += written;
    return maybe_sync();
}

bool PosixPartialFile::sync() {
    if (!file) {
        return false;
    }
    if (!flush_write_buf()) {
        return false;
    }
    return sync_flushed();
}

void PosixPartialFile::release_file() {
    file.reset();
}

bool PosixPartialFile::flush_write_buf() {
    if (write_buf_offset == 0) {
        return true;
    }
    if (!file) {
        return false;
    }

    debug_assert(ftell(file.get()) == static_cast<long>(current_offset - write_buf_offset));
    size_t written = fwrite(write_buf, 1, write_buf_offset, file.get());
    if (written != write_buf_offset) {
        log_error(transfers, "fwrite %u/%u: errno %d", (unsigned)written, (unsigned)write_buf_offset, errno);
        file.reset();
        return false;
    }
    write_buf_offset = 0;
    return true;
}

bool PosixPartialFile::sync_flushed() {
    if (!file) {
        return false;
    }
    if (fflush(file.get()) != 0) {
        return false;
    }
    // Commit littlefs data and metadata: data is durable only after this, an
    // unsynced file reverts to its last-synced size on power loss
    if (fsync(fileno(file.get())) != 0) {
        return false;
    }
    const size_t flushed_end = current_offset - write_buf_offset;
    if (flushed_end > synced_offset) {
        extend_valid_part({ synced_offset, flushed_end });
        synced_offset = flushed_end;
        notify_written();
        log_progress();
    }
    return true;
}

bool PosixPartialFile::maybe_sync() {
    // Commit on crossing a sync_interval multiple (not "interval since last
    // sync") to keep the frontier erase-block-aligned; the overshoot past
    // the boundary is at most one buffer instead of accumulating
    if (current_offset != state.total_size
        && current_offset / sync_interval == synced_offset / sync_interval) {
        return true;
    }
    if (!flush_write_buf()) {
        return false;
    }
    return sync_flushed();
}

PartialFile::BufferPeek PosixPartialFile::get_current_buffer(bool /*blocking_wait*/) {
    if (!file) {
        return WriteError {};
    }
    if (current_offset >= state.total_size) {
        log_error(transfers, "Out of range: %u/%u", (unsigned)current_offset, (unsigned)state.total_size);
        return OutOfRange {};
    }
    return BufferAndSizes {
        .buffer = write_buf,
        .offset = write_buf_offset,
        .size = write_buf_size,
    };
}

bool PosixPartialFile::advance_written(size_t by) {
    if (current_offset + by > state.total_size) {
        log_error(transfers, "PosixPartialFile: write past end (%u + %u > %u)",
            (unsigned)current_offset, (unsigned)by, (unsigned)state.total_size);
        return false;
    }
    write_buf_offset += by;
    current_offset += by;

    if (write_buf_offset >= write_buf_size && !flush_write_buf()) {
        return false;
    }
    return maybe_sync();
}

} // namespace transfers
