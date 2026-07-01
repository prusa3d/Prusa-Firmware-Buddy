#include <common/mt29f_flash.hpp>

#include <common/spi_flash_bus.hpp>
#include <hwio_pindef.h>
#include <logging/log.hpp>
#include <algorithm>

LOG_COMPONENT_DEF(MT29F, logging::Severity::info);

namespace {

enum Cmd : uint8_t {
    cmd_reset = 0xff,
    cmd_read_id = 0x9f,
    cmd_get_feature = 0x0f,
    cmd_set_feature = 0x1f,
    cmd_write_enable = 0x06,
    cmd_page_read = 0x13,
    cmd_read_from_cache = 0x0b,
    cmd_program_load = 0x02,
    cmd_program_execute = 0x10,
    cmd_block_erase = 0xd8,
};

enum FeatureAddr : uint8_t {
    feat_block_lock = 0xa0,
    feat_config = 0xb0,
    feat_status = 0xc0,
    feat_die_select = 0xd0,
};

enum Status : uint8_t {
    status_oip = 0x01,
    status_wel = 0x02,
    status_e_fail = 0x04,
    status_p_fail = 0x08,
};

enum EccStatus : uint8_t {
    ecc_mask = 0x70,
    ecc_no_error = 0x00,
    ecc_1_3_corrected = 0x10,
    ecc_uncorrectable = 0x20,
    ecc_4_6_corrected = 0x30,
    ecc_7_8_corrected = 0x50,
};

constexpr uint8_t config_ecc_en = 0x10;

constexpr uint8_t mfrid_micron = 0x2c;
constexpr uint8_t devid_8g_3v3 = 0x46;

/// Encode block/page into a 3-byte row address.
/// Layout: 7 dummy bits | 11-bit block | 6-bit page
void encode_row_address(uint8_t buf[3], uint32_t block, uint32_t page) {
    uint32_t row = (block << 6) | page;
    buf[0] = (row >> 16) & 0x01;
    buf[1] = (row >> 8) & 0xff;
    buf[2] = row & 0xff;
}

/// Encode byte offset into a 2-byte column address.
/// Layout: 3 dummy bits | 13-bit column
void encode_col_address(uint8_t buf[2], uint16_t col) {
    buf[0] = (col >> 8) & 0x1f;
    buf[1] = col & 0xff;
}

} // namespace

Mt29fFlash &Mt29fFlash::instance() {
    static Mt29fFlash instance(SpiFlashBus::instance(), buddy::hw::internal_storage_flash_cs);
    return instance;
}

Mt29fFlash::Mt29fFlash(SpiFlashBus &bus, const buddy::hw::OutputPin &cs)
    : bus(bus)
    , cs(cs) {}

bool Mt29fFlash::init() {
    bus.init();
    bus.lock();
    bus.select(cs);
    bus.send_byte(cmd_reset);
    bus.deselect(cs);
    unlock_bus();

    // tRST max ~5us for read, 500us worst case
    if (!wait_busy()) {
        log_error(MT29F, "reset timeout");
        return false;
    }

    // Read ID: 1 dummy byte + 2 data bytes
    bus.lock();
    bus.select(cs);
    uint8_t cmd[2] = { cmd_read_id, 0x00 }; // opcode + dummy
    bus.send(cmd, sizeof(cmd));
    uint8_t mfr = bus.receive_byte();
    uint8_t dev = bus.receive_byte();
    bus.deselect(cs);

    if (mfr != mfrid_micron || dev != devid_8g_3v3) {
        log_error(MT29F, "unexpected ID: mfr=0x%02x dev=0x%02x", mfr, dev);
        unlock_bus();
        return false;
    }

    // Unlock all blocks and verify ECC on both dies
    for (uint8_t die = 0; die < num_dies; die++) {
        current_die = 0xff; // force select
        select_die(die);
        set_feature(feat_block_lock, 0x00);

        uint8_t cfg = get_feature(feat_config);
        if (!(cfg & config_ecc_en)) {
            set_feature(feat_config, cfg | config_ecc_en);
        }
    }

    select_die(0);
    unlock_bus();
    initialized = true;
    return true;
}

void Mt29fFlash::read(uint32_t addr, uint8_t *data, uint32_t len) {
    while (len > 0) {
        auto a = decompose(addr);
        uint16_t remaining_in_page = page_size - a.col;
        uint16_t chunk = std::min(static_cast<uint32_t>(remaining_in_page), len);

        read_page(a, data, chunk);
        if (current_error) {
            return;
        }

        addr += chunk;
        data += chunk;
        len -= chunk;
    }
}

void Mt29fFlash::program(uint32_t addr, const uint8_t *data, uint32_t len) {
    while (len > 0) {
        auto a = decompose(addr);
        uint16_t remaining_in_page = page_size - a.col;
        uint16_t chunk = std::min(static_cast<uint32_t>(remaining_in_page), len);

        program_page(a, data, chunk);
        if (current_error) {
            return;
        }

        addr += chunk;
        data += chunk;
        len -= chunk;
    }
}

void Mt29fFlash::erase_block(uint32_t addr) {
    auto a = decompose(addr);

    bus.lock();
    select_die(a.die);
    write_enable();
    uint8_t cmd[4];
    cmd[0] = cmd_block_erase;
    encode_row_address(&cmd[1], a.block, 0);
    bus.select(cs);
    bus.send(cmd, sizeof(cmd));
    bus.deselect(cs);
    unlock_bus();

    if (!wait_busy()) {
        return;
    }

    bus.lock();
    uint8_t status = get_feature(feat_status);
    unlock_bus();
    if (status & status_e_fail) {
        set_error(HAL_ERROR);
    }
}

void Mt29fFlash::chip_erase() {
    // The MT29F ships with factory-marked bad blocks and develops more
    // over its life, so erase failures on individual blocks are expected:
    // skip them (littlefs maps around bad blocks) and only report
    // whole-chip failure.
    uint32_t failed = 0;
    for (uint8_t die = 0; die < num_dies; die++) {
        for (uint32_t block = 0; block < blocks_per_die; block++) {
            erase_block((die * blocks_per_die + block) * block_size);
            if (fetch_error()) {
                failed++;
            }
        }
    }
    if (failed >= num_dies * blocks_per_die) {
        set_error(HAL_ERROR);
    } else if (failed > 0) {
        log_warning(MT29F, "Chip erase: skipped %lu bad blocks", (unsigned long)failed);
    }
}

void Mt29fFlash::set_error(int error) {
    if (!current_error) {
        current_error = error;
    }
}

int Mt29fFlash::fetch_error() {
    int err = current_error;
    current_error = 0;
    return err;
}

/// Propagate any SPI bus error to per-instance error and release the bus.
void Mt29fFlash::unlock_bus() {
    if (int err = bus.fetch_error()) {
        set_error(err);
    }
    bus.unlock();
}

void Mt29fFlash::select_die(uint8_t die) {
    if (die == current_die) {
        return;
    }
    set_feature(feat_die_select, die ? 0x40 : 0x00);
    current_die = die;
}

void Mt29fFlash::write_enable() {
    bus.select(cs);
    bus.send_byte(cmd_write_enable);
    bus.deselect(cs);
}

uint8_t Mt29fFlash::get_feature(uint8_t addr) {
    bus.select(cs);
    uint8_t cmd[2] = { cmd_get_feature, addr };
    bus.send(cmd, sizeof(cmd));
    uint8_t val = bus.receive_byte();
    bus.deselect(cs);
    return val;
}

void Mt29fFlash::set_feature(uint8_t addr, uint8_t value) {
    bus.select(cs);
    uint8_t cmd[3] = { cmd_set_feature, addr, value };
    bus.send(cmd, sizeof(cmd));
    bus.deselect(cs);
}

/// Poll until the device is ready.
/// Self-contained: acquires and releases the bus lock per poll iteration
/// so that other devices on the shared SPI bus can interleave.
/// Caller must NOT hold the bus lock.
bool Mt29fFlash::wait_busy() {
    // tRD max is 25us (ECC off) or 115us (ECC on)
    // tPROG max is 600us
    // tBERS max is 10ms
    // At ~21MHz SPI, each status poll takes ~2us, so 10000 loops is plenty.
    for (uint32_t i = 0; i < 10000; i++) {
        bus.lock();
        uint8_t status = get_feature(feat_status);
        unlock_bus();
        if (!(status & status_oip)) {
            return true;
        }
    }
    set_error(HAL_TIMEOUT);
    return false;
}

bool Mt29fFlash::check_ecc() {
    uint8_t status = get_feature(feat_status);
    switch (status & ecc_mask) {
    case ecc_no_error:
    case ecc_1_3_corrected:
    case ecc_4_6_corrected:
    case ecc_7_8_corrected:
        return true;
    case ecc_uncorrectable:
    default:
        set_error(HAL_ERROR);
        return false;
    }
}

Mt29fFlash::Address Mt29fFlash::decompose(uint32_t addr) {
    uint32_t page_index = addr / page_size;
    uint32_t pages_per_die = blocks_per_die * pages_per_block;
    return {
        .die = static_cast<uint8_t>(page_index / pages_per_die),
        .block = (page_index % pages_per_die) / pages_per_block,
        .page = (page_index % pages_per_die) % pages_per_block,
        .col = static_cast<uint16_t>(addr % page_size),
    };
}

void Mt29fFlash::read_page(const Address &a, uint8_t *data, uint16_t len) {
    // PAGE READ: transfer page from array to internal cache
    bus.lock();
    select_die(a.die);
    uint8_t cmd_page[4];
    cmd_page[0] = cmd_page_read;
    encode_row_address(&cmd_page[1], a.block, a.page);
    bus.select(cs);
    bus.send(cmd_page, sizeof(cmd_page));
    bus.deselect(cs);
    unlock_bus();

    // Wait for internal transfer (bus free for other devices)
    if (!wait_busy()) {
        return;
    }

    // READ FROM CACHE: read data out of cache register
    bus.lock();
    if (!check_ecc()) {
        unlock_bus();
        log_error(MT29F, "ECC uncorrectable, block %lu page %lu",
            static_cast<unsigned long>(a.block), static_cast<unsigned long>(a.page));
        return;
    }
    uint8_t cmd_cache[4];
    cmd_cache[0] = cmd_read_from_cache;
    encode_col_address(&cmd_cache[1], a.col);
    cmd_cache[3] = 0x00; // dummy byte
    bus.select(cs);
    bus.send(cmd_cache, sizeof(cmd_cache));
    bus.receive(data, len);
    bus.deselect(cs);
    unlock_bus();
}

void Mt29fFlash::program_page(const Address &a, const uint8_t *data, uint16_t len) {
    // PROGRAM LOAD + EXECUTE: load data and start programming
    bus.lock();
    select_die(a.die);
    write_enable();

    uint8_t cmd_load[3];
    cmd_load[0] = cmd_program_load;
    encode_col_address(&cmd_load[1], a.col);
    bus.select(cs);
    bus.send(cmd_load, sizeof(cmd_load));
    bus.send(data, len);
    bus.deselect(cs);

    uint8_t cmd_exec[4];
    cmd_exec[0] = cmd_program_execute;
    encode_row_address(&cmd_exec[1], a.block, a.page);
    bus.select(cs);
    bus.send(cmd_exec, sizeof(cmd_exec));
    bus.deselect(cs);
    unlock_bus();

    // Wait for programming to complete (bus free for other devices)
    if (!wait_busy()) {
        return;
    }

    bus.lock();
    uint8_t status = get_feature(feat_status);
    unlock_bus();
    if (status & status_p_fail) {
        set_error(HAL_ERROR);
    }
}
