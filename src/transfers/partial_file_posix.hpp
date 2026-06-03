/// @file
#pragma once

#include "partial_file.hpp"
#include <common/unique_file_ptr.hpp>

namespace transfers {

/// POSIX-based partial file implementation using fwrite/fseek.
/// Used for non-FAT filesystems (e.g. littlefs on NAND).
class PosixPartialFile : public PartialFile {
public:
    PosixPartialFile(unique_file_ptr file, State state);
    ~PosixPartialFile() override;

    /// Construct from a file already opened by the PartialFile factory.
    static Result create(const char *path, unique_file_ptr file, size_t size);
    static Result open(const char *path, unique_file_ptr file, State state);

    bool supports_random_write() const override { return false; }
    bool seek(size_t offset) override;
    size_t tell() override { return current_offset; }
    bool write(const uint8_t *data, size_t size) override;
    bool sync() override;
    void release_file() override;

    BufferPeek get_current_buffer(bool blocking_wait) override;
    bool advance_written(size_t by) override;

private:
    unique_file_ptr file;
    size_t current_offset = 0;

    /// Data below this offset is fsync-durable and covered by valid parts.
    size_t synced_offset = 0;

    /// Granularity of the durable frontier, one NAND erase block.
    ///
    /// Must stay a multiple of the erase block: littlefs CTZ files copy the
    /// partial last block byte-by-byte into a fresh block on the first write
    /// after a sync (lfs_ctz_extend), so an unaligned sync costs up to a
    /// whole block of copying. Block-aligned syncs take the no-copy path.
    static constexpr size_t sync_interval = 256 * 1024;

    static constexpr size_t write_buf_size = 1024;
    uint8_t write_buf[write_buf_size];
    size_t write_buf_offset = 0;

    /// fwrite the staging buffer. Does not extend valid parts.
    bool flush_write_buf();

    /// fflush + fsync, then extend the valid parts over the flushed data.
    bool sync_flushed();

    /// Advance the durable frontier when due (interval crossed or file complete).
    bool maybe_sync();
};

} // namespace transfers
