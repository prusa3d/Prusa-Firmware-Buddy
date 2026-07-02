/// @file
/// Generic littlefs block device backed by a flash storage driver.
///
/// The Storage type must provide:
///   static constexpr uint32_t prog_size
///   void read(uint32_t addr, uint8_t *data, uint32_t len)
///   void program(uint32_t addr, const uint8_t *data, uint32_t len)
///   void erase_block(uint32_t addr)
///   int fetch_error()
///   uint32_t erase_block_size() const
///   uint32_t block_count() const
#pragma once

#include "lfs.h"
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <freertos/mutex.hpp>
#include <logging/log.hpp>
#include <printers.h>

LOG_COMPONENT_REF(FileSystem);

template <typename Storage>
class LittlefsFlashDevice {
public:
    /// @param storage  Reference to the flash driver instance
    /// @param addr_offset  Byte offset into the flash where the filesystem starts (default 0)
    /// @param name_max  Max filename length (0 = littlefs default). Stored in the
    ///   superblock on format; on mount it acts as an upper bound - a filesystem
    ///   formatted with a larger value fails to mount (and init() reformats it).
    LittlefsFlashDevice(Storage &storage, uint32_t addr_offset = 0, uint32_t name_max = 0)
        : storage(storage)
        , addr_offset(addr_offset)
        , name_max(name_max) {}

    /// Mount the filesystem (formats on first boot). Returns lfs handle or nullptr on failure.
    lfs_t *init() {
        config = lfs_config {
            .context = this,
            .read = read_cb,
            .prog = prog_cb,
            .erase = erase_cb,
            .sync = sync_cb,
            .lock = lock_cb,
            .unlock = unlock_cb,

            .read_size = 1,
            .prog_size = Storage::prog_size,
            .block_size = storage.erase_block_size(),
            .block_count = storage.block_count() - (addr_offset / storage.erase_block_size()),
            .block_cycles = 500,
            .cache_size = cache_buf_size,
            .lookahead_size = lookahead_size,
            .read_buffer = read_buffer,
            .prog_buffer = prog_buffer,
            .lookahead_buffer = lookahead_buffer,
            .name_max = name_max,
            .file_max = 0,
            .attr_max = 0,
            // Bound metadata compaction time on large erase blocks
            .metadata_max = std::min(storage.erase_block_size(), 32768lu),
        };

        int err = lfs_mount(&lfs, &config);
        if (err == LFS_ERR_IO) {
            // Possibly a transient driver error; formatting now could destroy
            // an intact filesystem. The caller may retry or escalate.
            log_error(FileSystem, "littlefs mount failed with an IO error");
            return nullptr;
        }
        if (err) {
            // Not a littlefs, corrupted, or an incompatible configuration
            // (e.g. lowered name_max) - reformatting is the upgrade path
            log_info(FileSystem, "littlefs mount failed (%d), formatting...", err);
            err = lfs_format(&lfs, &config);
            if (err) {
                log_error(FileSystem, "littlefs format failed: %d", err);
                return nullptr;
            }
            err = lfs_mount(&lfs, &config);
            if (err) {
                log_error(FileSystem, "littlefs remount failed: %d", err);
                return nullptr;
            }
        }

        return &lfs;
    }

private:
    Storage &storage;
    uint32_t addr_offset;
    uint32_t name_max;

    lfs_t lfs;
    lfs_config config;
    freertos::Mutex lfs_mutex;

    static constexpr lfs_size_t lookahead_size = 16;
#if PRINTER_IS_PRUSA_MINI()
    // need to save RAM on MINI, also PNG draw is faster due smaller and more efficient display
    // MINI doesn't have /internal storage, so this only affects /system
    static constexpr lfs_size_t cache_buf_size = 128;
#else
    // PNG draws about 4% faster compared to cache_size 128
    // for NAND (/internal) the cache must hold whole program pages
    static constexpr lfs_size_t cache_buf_size = std::max<lfs_size_t>(Storage::prog_size, 512);
#endif
    // littlefs requires this, but its own check is compiled out by LFS_NO_ASSERT
    static_assert(cache_buf_size % Storage::prog_size == 0, "cache_size must be a multiple of prog_size");
    std::byte read_buffer[cache_buf_size];
    std::byte prog_buffer[cache_buf_size];
    std::byte lookahead_buffer[lookahead_size];

    static LittlefsFlashDevice &self(const lfs_config *c) {
        return *static_cast<LittlefsFlashDevice *>(c->context);
    }

    uint32_t get_address(lfs_block_t block, lfs_off_t off) const {
        return addr_offset + block * config.block_size + off;
    }

    /// A corrupted block pointer would address flash outside the filesystem
    /// region (the chip is shared with other consumers). littlefs's own
    /// bound asserts are compiled out by LFS_NO_ASSERT.
    static bool out_of_bounds(const lfs_config *c, lfs_block_t block, lfs_off_t off, lfs_size_t size) {
        if (block >= c->block_count || size > c->block_size || off > c->block_size - size) {
            log_error(FileSystem, "lfs access out of bounds: blk %lu off %lu size %u",
                (unsigned long)block, (unsigned long)off, (unsigned)size);
            return true;
        }
        return false;
    }

    static int read_cb(const lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
        if (out_of_bounds(c, block, off, size)) {
            return LFS_ERR_INVAL;
        }
        auto &dev = self(c);
        uint32_t addr = dev.get_address(block, off);
        dev.storage.read(addr, static_cast<uint8_t *>(buffer), size);
        if (int err = dev.storage.fetch_error()) {
            log_error(FileSystem, "lfs read blk %lu: %d", (unsigned long)block, err);
            return LFS_ERR_IO;
        }
        return 0;
    }

    static int prog_cb(const lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
        if (out_of_bounds(c, block, off, size)) {
            return LFS_ERR_INVAL;
        }
        auto &dev = self(c);
        uint32_t addr = dev.get_address(block, off);
        dev.storage.program(addr, static_cast<const uint8_t *>(buffer), size);
        if (int err = dev.storage.fetch_error()) {
            log_error(FileSystem, "lfs prog blk %lu: %d", (unsigned long)block, err);
            return LFS_ERR_IO;
        }
        return 0;
    }

    static int erase_cb(const lfs_config *c, lfs_block_t block) {
        if (out_of_bounds(c, block, 0, 0)) {
            return LFS_ERR_INVAL;
        }
        auto &dev = self(c);
        uint32_t addr = dev.get_address(block, 0);
        dev.storage.erase_block(addr);
        if (int err = dev.storage.fetch_error()) {
            log_error(FileSystem, "lfs erase blk %lu: %d", (unsigned long)block, err);
            return LFS_ERR_IO;
        }
        return 0;
    }

    static int sync_cb([[maybe_unused]] const lfs_config *c) {
        return 0;
    }

    static int lock_cb(const lfs_config *c) {
        self(c).lfs_mutex.lock();
        return 0;
    }

    static int unlock_cb(const lfs_config *c) {
        self(c).lfs_mutex.unlock();
        return 0;
    }
};
