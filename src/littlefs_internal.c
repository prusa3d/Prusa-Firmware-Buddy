#include "littlefs_internal.h"
#include "w25x.h"

#define BLOCK_SIZE 4096

// 512 kB for dump (128 x 4kB)
#define DUMP_BLOCK_COUNT 128

// Address offset to skip area reserved for the dump
#define ADDR_OFFSET (BLOCK_SIZE * DUMP_BLOCK_COUNT)

static lfs_t lfs;

// Read a region in a block. Negative error codes are propogated
// to the user.
static int read(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, void *buffer, lfs_size_t size);

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int prog(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, const void *buffer, lfs_size_t size);

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int erase(const struct lfs_config *c, lfs_block_t block);

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int sync(const struct lfs_config *c);

// configuration of the filesystem is provided by this struct
static struct lfs_config littlefs_config = {
    // block device operations
    .read = read,
    .prog = prog,
    .erase = erase,
    .sync = sync,

    // block device configuration
    .read_size = 1,
    .prog_size = 1,
    .block_size = BLOCK_SIZE,
    .block_count = 0, // to be initialized at runtime
    .cache_size = 128,
    .lookahead_size = 16,
    .block_cycles = 500,
};

static uint32_t get_address(lfs_block_t block, lfs_off_t offset) {
    return ADDR_OFFSET + block * littlefs_config.block_size + offset;
}

static bool address_is_valid(uint32_t address) {
    return address >= ADDR_OFFSET && address <= (w25x_get_sector_count() * BLOCK_SIZE);
}

static int read(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, void *buffer, lfs_size_t size) {

    uint32_t addr = get_address(block, off);
    if (!address_is_valid(addr)) {
        return LFS_ERR_INVAL;
    }

    w25x_rd_data(addr, buffer, size);

    if (w25x_fetch_error() != 0)
        return LFS_ERR_IO;

    return 0;
}

static int prog(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, const void *buffer, lfs_size_t size) {

    uint32_t addr = get_address(block, off);
    if (!address_is_valid(addr)) {
        return LFS_ERR_INVAL;
    }

    w25x_page_program(addr, (uint8_t *)buffer, size);

    if (w25x_fetch_error() != 0)
        return LFS_ERR_IO;

    return 0;
}

static int erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = get_address(block, 0);
    if (!address_is_valid(addr)) {
        return LFS_ERR_INVAL;
    }

    w25x_sector_erase(addr);

    if (w25x_fetch_error() != 0)
        return LFS_ERR_IO;

    return 0;
}

static int sync(__attribute__((unused)) const struct lfs_config *c) {
    return 0;
}

lfs_t *littlefs_internal_init() {
    // setup flash size
    littlefs_config.block_count = w25x_get_sector_count() - DUMP_BLOCK_COUNT;

    // mount the filesystem
    int err = lfs_mount(&lfs, &littlefs_config);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        err = lfs_format(&lfs, &littlefs_config);
        if (err) {
            return NULL;
        }
        err = lfs_mount(&lfs, &littlefs_config);
        if (err) {
            return NULL;
        }
    }

    return &lfs;
}

struct lfs_config *littlefs_internal_config_get() {
    if (littlefs_config.block_count != 0) {
        return &littlefs_config;
    } else {
        return NULL;
    }
}
