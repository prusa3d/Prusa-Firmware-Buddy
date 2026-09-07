#include <common/w25x.hpp>

#include <common/spi_flash_bus.hpp>
#include "timing_precise.hpp"
#include <logging/log.hpp>
#include <bsod.h>
#include <stdlib.h>

LOG_COMPONENT_DEF(W25X, logging::Severity::info);

namespace {

enum Cmd : uint8_t {
    cmd_enable_wr = 0x06,
    cmd_enable_wr_vsr = 0x50,
    cmd_disable_wr = 0x04,
    cmd_rd_status1_reg = 0x05,
    cmd_rd_status2_reg = 0x35,
    cmd_wr_status1_reg = 0x01,
    cmd_rd_data = 0x03,
    cmd_rd_fast = 0x0b,
    cmd_rd_fast_d_o = 0x3b,
    cmd_rd_fast_d_io = 0xbb,
    cmd_page_program = 0x02,
    cmd_sector_erase = 0x20,
    cmd_block32_erase = 0x52,
    cmd_block64_erase = 0xd8,
    cmd_chip_erase = 0xc7,
    cmd_chip_erase2 = 0x60,
    cmd_erase_program_suspend = 0x75,
    cmd_erase_program_resume = 0x7a,
    cmd_pwr_down = 0xb9,
    cmd_pwr_down_rel = 0xab,
    cmd_mfrid_devid = 0x90,
    cmd_mfrid_devid_d = 0x92,
    cmd_jedec_id = 0x9f,
    cmd_rd_uid = 0x4b,
};

enum Status1 : uint8_t {
    status1_busy = 0x01,
    status1_wel = 0x02,
    status1_bp0 = 0x04,
    status1_bp1 = 0x08,
    status1_bp2 = 0x10,
    status1_tb = 0x20,
    status1_sec = 0x40,
    status1_srp0 = 0x80,
};

enum Status2 : uint8_t {
    status2_srp1 = 0x01,
    status2_qe = 0x02,
    status2_reserved = 0x04,
    status2_lb1 = 0x08,
    status2_lb2 = 0x10,
    status2_lb3 = 0x20,
    status2_cmp = 0x40,
    status2_sus = 0x80,
};

struct CmdWithAddress {
    uint8_t buffer[4];
};

constexpr uint16_t page_size = 256;

constexpr uint8_t mfrid = 0xEF;
constexpr uint8_t devid = 0x13;
constexpr uint8_t devid_new = 0x16;

constexpr uint32_t t_sus_ns = 20000;

constexpr uint32_t max_wait_loops() {
    constexpr uint32_t max_bitrate_hz = 133000000;
    constexpr uint32_t bits_per_status_read = 16;
    constexpr uint64_t nanoseconds_per_second = 1000000000ull;
    constexpr uint32_t transfer_duration_ns = bits_per_status_read * nanoseconds_per_second / max_bitrate_hz;
    constexpr uint32_t cs_duration_ns = SpiFlashBus::cs_deselect_delay_ns + SpiFlashBus::cs_select_delay_ns;

    constexpr uint32_t max_operation_duration_s = 100; // W25Q64JV Chip erase
    return (nanoseconds_per_second / (transfer_duration_ns + cs_duration_ns) * max_operation_duration_s);
}

CmdWithAddress cmd_with_address(uint8_t cmd, uint32_t addr) {
    return { { cmd,
        reinterpret_cast<uint8_t *>(&addr)[2],
        reinterpret_cast<uint8_t *>(&addr)[1],
        reinterpret_cast<uint8_t *>(&addr)[0] } };
}

} // namespace

W25xFlash &W25xFlash::instance() {
    static W25xFlash instance(SpiFlashBus::instance(), buddy::hw::extFlashCs);
    return instance;
}

W25xFlash::W25xFlash(SpiFlashBus &bus, const buddy::hw::OutputPin &cs)
    : bus(bus)
    , cs(cs) {}

void W25xFlash::lock_erase() {
    if (!bus.is_scheduler_stopped()) {
        erase_mutex.lock();
    }
}

void W25xFlash::unlock_erase() {
    if (!bus.is_scheduler_stopped()) {
        erase_mutex.unlock();
    }
}

bool W25xFlash::try_lock_erase() {
    if (bus.is_scheduler_stopped()) {
        return true;
    }
    return erase_mutex.try_lock();
}

void W25xFlash::select() {
    bus.select(cs);
}

void W25xFlash::deselect() {
    bus.deselect(cs);
}

void W25xFlash::write_enable() {
    select();
    bus.send_byte(cmd_enable_wr);
    deselect();
}

uint8_t W25xFlash::read_status1() {
    select();
    bus.send_byte(cmd_rd_status1_reg);
    uint8_t status = bus.receive_byte();
    deselect();
    return status;
}

uint8_t W25xFlash::read_status2() {
    select();
    bus.send_byte(cmd_rd_status2_reg);
    uint8_t status = bus.receive_byte();
    deselect();
    return status;
}

bool W25xFlash::is_suspended() {
    return read_status2() & status2_sus;
}

bool W25xFlash::wait_busy() {
    uint32_t loop_counter = 0;
    while (read_status1() & status1_busy) {
        ++loop_counter;
        if (loop_counter > max_wait_loops()) {
            return false;
        }
    }
    return true;
}

bool W25xFlash::wait_erase() {
    uint32_t loop_counter = 0;
    uint8_t status;

    do {
        bus.lock();
        ++loop_counter;
        status = read_status1();
        if (!(status & status1_busy)) {
            is_erasing = false;
        }
        unlock_bus();
        if (loop_counter > max_wait_loops()) {
            return false;
        }
    } while (status & status1_busy);

    return true;
}

bool W25xFlash::check_id(uint8_t *out_devid) {
    select();
    CmdWithAddress cmdwa = cmd_with_address(cmd_mfrid_devid, 0ul);
    bus.send(cmdwa.buffer, sizeof(cmdwa.buffer));
    uint8_t mfr = bus.receive_byte();
    uint8_t dev = bus.receive_byte();
    deselect();
    if (out_devid) {
        *out_devid = dev;
    }
    return mfr == mfrid && (dev == devid || dev == devid_new);
}

void W25xFlash::suspend_erase() {
    select();
    bus.send_byte(cmd_erase_program_suspend);
    deselect();
    // W25Q guarantees to be available in tSUS
    // alternatively busy status can be polled
    delay_ns_precise<t_sus_ns>();
}

void W25xFlash::resume_erase() {
    select();
    bus.send_byte(cmd_erase_program_resume);
    deselect();
    // Assure that suspend is not called earlier than in tSUS
    // after resume
    delay_ns_precise<t_sus_ns>();
}

/// Program a single page. If !high_priority, acquires/releases locks per call
/// so higher-priority callers can interleave between pages.
void W25xFlash::program_page(uint32_t addr, const uint8_t *data, uint16_t cnt, bool high_priority) {
    if (!high_priority) {
        lock_erase();
        bus.lock();
    }
    write_enable();
    select();
    CmdWithAddress cmdwa = cmd_with_address(cmd_page_program, addr);
    bus.send(cmdwa.buffer, sizeof(cmdwa.buffer));
    bus.send(data, cnt);
    deselect();
    if (!wait_busy()) {
        set_error(HAL_TIMEOUT);
    }
    if (!high_priority) {
        unlock_bus();
        unlock_erase();
    }
}

/// Split a write into page-aligned chunks.
void W25xFlash::split_page_program(uint32_t addr, const uint8_t *data, uint32_t cnt, bool high_priority) {
    // Write unaligned part first
    uint32_t addr_align = addr % page_size;
    if (addr_align != 0) {
        uint32_t cnt_align = page_size - addr_align;
        if (cnt_align >= cnt) {
            program_page(addr, data, cnt, high_priority);
            return;
        }
        program_page(addr, data, cnt_align, high_priority);
        addr += cnt_align;
        data += cnt_align;
        cnt -= cnt_align;
    }

    // Write all full pages
    while (cnt >= page_size) {
        program_page(addr, data, page_size, high_priority);
        addr += page_size;
        data += page_size;
        cnt -= page_size;
    }

    // Write the remaining data
    if (cnt > 0) {
        program_page(addr, data, cnt, high_priority);
    }
}

void W25xFlash::erase(uint8_t cmd, uint32_t addr) {
    lock_erase();
    bus.lock();
    is_erasing = true;
    write_enable();
    select();
    CmdWithAddress cmdwa = cmd_with_address(cmd, addr);
    bus.send(cmdwa.buffer, sizeof(cmdwa.buffer));
    deselect();
    unlock_bus();
    if (!wait_erase()) {
        set_error(HAL_TIMEOUT);
    }
    unlock_erase();
}

bool W25xFlash::init_internal() {
    deselect();

    if (!wait_busy()) {
        return false;
    }

    if (is_suspended()) {
        resume_erase();
        if (!wait_busy()) {
            return false;
        }
    }

    is_erasing = false;

    if (!check_id(&device_id)) {
        return false;
    }

    return true;
}

bool W25xFlash::init() {
    bus.init();
    return init_internal();
}

bool W25xFlash::reinit_before_crash_dump() {
    bus.init();

    // Sets scheduler_stopped flag on the bus, making all lock/unlock
    // calls no-ops. Safe because after this point only one thread of
    // execution exists.
    bus.reinit_for_crash_dump();

    return init_internal();
}

uint32_t W25xFlash::block_count() const {
    if (device_id == devid) {
        return 256;
    } else if (device_id == devid_new) {
        return 2048;
    } else {
        abort();
    }
}

void W25xFlash::read(uint32_t addr, uint8_t *data, uint32_t len) {
    lock_erase();
    bus.lock();
    select();
    CmdWithAddress cmdwa = cmd_with_address(cmd_rd_data, addr);
    bus.send(cmdwa.buffer, sizeof(cmdwa.buffer));
    bus.receive(data, len);
    deselect();
    unlock_bus();
    unlock_erase();
}

void W25xFlash::program(uint32_t addr, const uint8_t *data, uint32_t len) {
    // For high priority address range (power panic) block erase can be
    // suspended and the bus is locked until all pages are written at once,
    // then the suspended erase operation is resumed.
    // For low priority address range the bus is locked only per page
    // so higher priority tasks can write / erase in between.
    const bool high_priority = (addr >= pp_start_address) && (addr < fs_start_address);

    bool erase_locked = false;

    if (high_priority) {
        erase_locked = try_lock_erase();
        bus.lock();
        if (is_erasing) {
            suspend_erase();
        }
    }

    split_page_program(addr, data, len, high_priority);

    if (high_priority) {
        if (is_erasing) {
            resume_erase();
        }
        unlock_bus();
        if (erase_locked) {
            unlock_erase();
        }
    }
}

void W25xFlash::erase_block(uint32_t addr) {
    erase(cmd_sector_erase, addr);
}

void W25xFlash::block32_erase(uint32_t addr) {
    erase(cmd_block32_erase, addr);
}

void W25xFlash::block64_erase(uint32_t addr) {
    erase(cmd_block64_erase, addr);
}

int W25xFlash::fetch_error() {
    int err = current_error;
    current_error = 0;
    return err;
}

/// Propagate any SPI bus error to per-instance error and release the bus.
void W25xFlash::unlock_bus() {
    if (int err = bus.fetch_error()) {
        set_error(err);
    }
    bus.unlock();
}

void W25xFlash::set_error(int error) {
    if (!current_error) {
        current_error = error;
    }
}
