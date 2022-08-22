#include "littlefs_bbf.h"
#include "bbf.hpp"
#include "log.h"

LOG_COMPONENT_REF(FileSystem);

static lfs_t lfs;

typedef struct {
    FILE *bbf;
    long data_offset;
} bbf_lfs_context_t;

static bbf_lfs_context_t bbf_context;

// configuration of the filesystem is provided by this struct
static struct lfs_config littlefs_config;
static int read(const struct lfs_config *c, lfs_block_t block,
    lfs_off_t off, void *buffer, lfs_size_t size) {

    long offset = bbf_context.data_offset + block * c->block_size + off;
    if (fseek(bbf_context.bbf, offset, SEEK_SET) != 0) {
        return LFS_ERR_IO;
    }

    if (fread(buffer, 1, size, bbf_context.bbf) != size) {
        return LFS_ERR_IO;
    }

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

    memset(&littlefs_config, 0, sizeof(littlefs_config));
    littlefs_config.block_count = block_count;
    littlefs_config.block_size = block_size;
    littlefs_config.read = read;

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

void littlefs_bbf_deinit(lfs_t *lfs) {
    lfs_unmount(lfs);
}

struct lfs_config *littlefs_internal_config_get() {
    if (littlefs_config.block_count != 0) {
        return &littlefs_config;
    } else {
        return NULL;
    }
}
