#include "littlefs.h"
#include "w25x.h"

#define BLOCK_SIZE  4096
#define BLOCK_COUNT 2048
#define FLASH_SIZE  (BLOCK_SIZE * BLOCK_COUNT)

// 512 kB for dump (128 x 4kB)
#define DUMP_BLOCK_COUNT 128

// Address offset to skip area reserved for the dump
#define ADDR_OFFSET (BLOCK_SIZE * DUMP_BLOCK_COUNT)

lfs_t lfs;

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
static const struct lfs_config cfg = {
    // block device operations
    .read = read,
    .prog = prog,
    .erase = erase,
    .sync = sync,

    // block device configuration
    .read_size = 1,
    .prog_size = 1,
    .block_size = BLOCK_SIZE,
    .block_count = BLOCK_COUNT - DUMP_BLOCK_COUNT,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

static int read(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, void *buffer, lfs_size_t size) {

    uint32_t addr = ADDR_OFFSET + block * c->block_size + off;
    if (addr >= FLASH_SIZE) {
        return LFS_ERR_INVAL;
    }

    w25x_rd_data(addr, buffer, size);
    return 0;
}

static int prog(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, const void *buffer, lfs_size_t size) {

    uint32_t addr = ADDR_OFFSET + block * c->block_size + off;
    if (addr >= FLASH_SIZE) {
        return LFS_ERR_INVAL;
    }

    w25x_page_program(addr, (uint8_t *)buffer, size);
    return 0;
}

static int erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = ADDR_OFFSET + block * c->block_size;
    if (addr >= FLASH_SIZE) {
        return LFS_ERR_INVAL;
    }
    w25x_sector_erase(addr);
    return 0;
}

static int sync(__attribute__((unused)) const struct lfs_config *c) {
    w25x_wait_busy();
    return 0;
}

lfs_t *littlefs_init() {
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        err = lfs_format(&lfs, &cfg);
        if (err) {
            return NULL;
        }
        err = lfs_mount(&lfs, &cfg);
        if (err) {
            return NULL;
        }
    }

    return &lfs;
}
