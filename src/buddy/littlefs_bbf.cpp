#include <buddy/littlefs_bbf.h>

#include <unistd.h>
#include <optional>
#include <errno.h>

#include "bbf.hpp"
#include <logging/log.hpp>
#include "bsod.h"
#include "bsod.h"

LOG_COMPONENT_REF(FileSystem);

static lfs_t lfs;

struct LruCache {
    struct Slot {
        static constexpr lfs_block_t INVALID_BLOCK_NR = -1;

        lfs_block_t block_nr;
        Slot *next;
        Slot *previous;
        uint8_t data[];
    };

    size_t block_size;
    size_t used_space;
    Slot *head;

    /**
     * @brief Size of the cache buffer in bytes.
     * @note Smaller cache means longer bbf installing from USB and almost unusable installing over debugger.
     *  USB installing times:
     *    12 blocks - 30 seconds
     *    10 blocks - 36 seconds
     *     8 blocks - 47 seconds
     *     6 blocks - 1 minute
     *     4 blocks - 1 minute 7 seconds
     *     2 blocks - 1 minute 7 seconds
     */
#if PRINTER_IS_PRUSA_MINI()
    static constexpr size_t CACHE_SIZE = 4 * (4096 + sizeof(LruCache::Slot)); // Mini doesn't have enough RAM, sorry mini
#else
    static constexpr size_t CACHE_SIZE = 12 * (4096 + sizeof(LruCache::Slot)); // Optimized for bbf with 4096 B blocks
#endif

    uint8_t *buffer; ///< Dynamically allocated space for block cache

    LruCache(size_t block_size)
        : block_size(block_size)
        , used_space(0)
        , head(nullptr) {
        buffer = new uint8_t[CACHE_SIZE];
    }

    ~LruCache() {
        delete[] buffer;
    }

    int slot_alloc_size() {
        return sizeof(Slot) + block_size;
    }

    Slot &alloc() {
        Slot *slot;
        if (used_space + slot_alloc_size() < CACHE_SIZE) {
            slot = reinterpret_cast<Slot *>(buffer + used_space);
            used_space += slot_alloc_size();
        } else {
            Slot *current = head;
            if (current == nullptr) {
                fatal_error(ErrCode::ERR_SYSTEM_BBF_ALLOCATION_FAILED);
            }
            while (current->next) {
                current = current->next;
            }
            unchain(current);
            slot = current;
        }

        slot->block_nr = Slot::INVALID_BLOCK_NR;
        slot->next = nullptr;
        slot->previous = nullptr;
        return *slot;
    }

    std::optional<Slot *> get(lfs_block_t block_nr) {
        Slot *current = head;
        while (current) {
            if (current->block_nr == block_nr) {
                return current;
            }
            current = current->next;
        }
        return std::nullopt;
    }

    bool has(lfs_block_t block_nr) {
        return get(block_nr).has_value();
    }

    void move_front(Slot *slot) {
        unchain(slot);
        slot->next = head;
        if (head) {
            head->previous = slot;
        }
        head = slot;
    }

    void unchain(Slot *slot) {
        if (head == slot) {
            head = slot->next;
        }

        if (slot->previous) {
            slot->previous->next = slot->next;
        }
        if (slot->next) {
            slot->next->previous = slot->previous;
        }
        slot->next = nullptr;
        slot->previous = nullptr;
    }
};

struct bbf_lfs_context_t {
    struct lfs_config littlefs_config;
    FILE *bbf;
    long data_offset;

    std::optional<LruCache> block_cache;
};

static bbf_lfs_context_t bbf_context;

// We can't use the standard read() directly function, as that is currently
// shadowed by lwip_read :(
extern "C" _ssize_t _read_r(struct _reent *r, int fileDesc, void *ptr, size_t len);
#define read(fd, ptr, len) _read_r(_REENT, fd, ptr, len)

static int _read(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, void *out_buffer, lfs_size_t size) {

    LruCache &cache = bbf_context.block_cache.value();

    if (bbf_context.block_cache.has_value() == false) {
        fatal_error(ErrCode::ERR_SYSTEM_BBF_INIT_FAILED);
    }

    LruCache::Slot *slot;
    if (!cache.has(block)) {
        slot = &cache.alloc();

        long block_offset = bbf_context.data_offset + block * c->block_size;
        int retval = fseek(bbf_context.bbf, block_offset, SEEK_SET);
        if (retval != 0) {
            log_error(FileSystem, "BBF: fseek to block %ld failed with retval %i, errno %i", block_offset, retval, errno);
            return LFS_ERR_IO;
        }
        retval = read(fileno(bbf_context.bbf), slot->data, c->block_size);
        if (static_cast<lfs_size_t>(retval) != c->block_size) {
            log_error(FileSystem, "BBF: read for block %ld failed with retval %i, errno %i", block_offset, retval, errno);
            return LFS_ERR_IO;
        }

        slot->block_nr = block;
    } else {
        slot = cache.get(block).value();
    }

    memcpy(out_buffer, slot->data + off, size);
    cache.move_front(slot);

    return 0;
}

lfs_t *littlefs_bbf_init(FILE *bbf, uint8_t bbf_tlv_entry) {
    uint32_t entry_length;

    uint32_t block_size;
    uint32_t block_count;

    buddy::bbf::TLVType entry = static_cast<buddy::bbf::TLVType>(bbf_tlv_entry);

    // read block size
    if (!buddy::bbf::seek_to_tlv_entry(bbf, buddy::bbf::block_size_for_image(entry), entry_length)) {
        log_error(FileSystem, "BBF: Failed to find block size entry");
        return nullptr;
    }
    if (fread(&block_size, 1, sizeof(block_size), bbf) != sizeof(block_size)) {
        log_error(FileSystem, "BBF: Failed to read block size entry");
        return nullptr;
    }

    // read block count
    if (!buddy::bbf::seek_to_tlv_entry(bbf, buddy::bbf::block_count_for_image(entry), entry_length)) {
        log_error(FileSystem, "BBF: Failed to find block count entry");
        return nullptr;
    }
    if (fread(&block_count, 1, sizeof(block_count), bbf) != sizeof(block_count)) {
        log_error(FileSystem, "BBF: Failed to read block count entry");
        return nullptr;
    }

    // find offset
    if (!buddy::bbf::seek_to_tlv_entry(bbf, entry, entry_length)) {
        log_error(FileSystem, "BBF: Failed to find main data entry");
        return nullptr;
    }
    long offset = ftell(bbf);

    bbf_context.bbf = bbf;
    bbf_context.data_offset = offset;

    /**
     * @note Allocating large amount of memory.
     *   Smaller cache increases time of installing from USB and makes installing over debugger unusable.
     *   This happens only before proper startup, so it shouldn't be a problem.
     */
    bbf_context.block_cache.emplace(block_size);

    struct lfs_config &littlefs_config = bbf_context.littlefs_config;
    memset(&littlefs_config, 0, sizeof(littlefs_config));
    littlefs_config.block_count = block_count;
    littlefs_config.block_size = block_size;
    littlefs_config.read = _read;

    littlefs_config.read_size = 1;
    littlefs_config.prog_size = 1;
    littlefs_config.cache_size = 16;
    littlefs_config.lookahead_size = 16;
    littlefs_config.block_cycles = 500;

    // mount the filesystem
    int err = lfs_mount(&lfs, &littlefs_config);
    if (err != LFS_ERR_OK) {
        return NULL;
    }

    return &lfs;
}

struct lfs_config *littlefs_internal_config_get() {
    if (bbf_context.littlefs_config.block_count != 0) {
        return &bbf_context.littlefs_config;
    } else {
        return NULL;
    }
}

void littlefs_bbf_deinit(lfs_t *lfs) {
    // Free a large chunk of heap by destroying LRU cache
    bbf_context.block_cache = std::nullopt;

    lfs_unmount(lfs);
}
