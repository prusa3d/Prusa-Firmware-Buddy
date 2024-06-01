#include "w25x.h"
#include "w25x_communication.h"
#include "timing_precise.hpp"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "bsod.h"
#include "hwio_pindef.h"
#include <stdlib.h>

LOG_COMPONENT_DEF(W25X, LOG_SEVERITY_DEBUG);

namespace {

enum {
    CMD_ENABLE_WR = 0x06,
    CMD_ENABLE_WR_VSR = 0x50,
    CMD_DISABLE_WR = 0x04,
    CMD_RD_STATUS1_REG = 0x05,
    CMD_RD_STATUS2_REG = 0x35,
    CMD_WR_STATUS1_REG = 0x01,
    CMD_RD_DATA = 0x03,
    CMD_RD_FAST = 0x0b,
    CMD_RD_FAST_D_O = 0x3b,
    CMD_RD_FAST_D_IO = 0xbb,
    CMD_PAGE_PROGRAM = 0x02,
    CMD_SECTOR_ERASE = 0x20,
    CMD_BLOCK32_ERASE = 0x52,
    CMD_BLOCK64_ERASE = 0xd8,
    CMD_CHIP_ERASE = 0xc7,
    CMD_CHIP_ERASE2 = 0x60,
    CMD_ERASE_PROGRAM_SUSPEND = 0x75,
    CMD_ERASE_PROGRAM_RESUME = 0x7a,
    CMD_PWR_DOWN = 0xb9,
    CMD_PWR_DOWN_REL = 0xab,
    CMD_MFRID_DEVID = 0x90,
    CMD_MFRID_DEVID_D = 0x92,
    CMD_JEDEC_ID = 0x9f,
    CMD_RD_UID = 0x4b,
};

typedef enum {
    W25X_STATUS_BUSY = 0x01,
    W25X_STATUS_WEL = 0x02,
    W25X_STATUS_BP0 = 0x04,
    W25X_STATUS_BP1 = 0x08,
    W25X_STATUS_BP2 = 0x10,
    W25X_STATUS_TB = 0x20,
    W25X_STATUS_SEC = 0x40,
    W25X_STATUS_SRP0 = 0x80,
} w25x_status1_t;

typedef enum {
    W25X_STATUS2_SRP1 = 0x01,
    W25X_STATUS2_QE = 0x02,
    W25X_STATUS2_RESERVED = 0x04,
    W25X_STATUS2_LB1 = 0x08,
    W25X_STATUS2_LB2 = 0x10,
    W25X_STATUS2_LB3 = 0x20,
    W25X_STATUS2_CMP = 0x40,
    W25X_STATUS2_SUS = 0x80,
} w25x_status2_t;

struct CmdWithAddress {
    uint8_t buffer[4];
};

class OptionalMutex {
public:
    bool acquired() { return m_acquired; }

    OptionalMutex(osMutexId mutex_id, bool wait = true)
        : m_mutex_id(mutex_id) {
        if (mutex_id) {
            auto result = osMutexWait(m_mutex_id, wait ? osWaitForever : 0);
            m_acquired = result == osOK;
            if (wait && !m_acquired) {
                bsod("osMutexWait forever failed.");
            }
        } else {
            m_acquired = true;
        }
    }

    ~OptionalMutex() {
        if (m_mutex_id && m_acquired) {
            if (osOK != osMutexRelease(m_mutex_id)) {
                bsod("osMutexRelease failed.");
            }
        }
    }

    OptionalMutex(const OptionalMutex &other) = delete;
    OptionalMutex &operator=(const OptionalMutex &other) = delete;

private:
    const osMutexId m_mutex_id;
    bool m_acquired;
};

constexpr uint16_t PAGE_SIZE = 256;

constexpr uint8_t MFRID = 0xEF;
constexpr uint8_t DEVID_8M = 0x13;
constexpr uint8_t DEVID_16M = 0x14;
constexpr uint8_t DEVID_32M = 0x15;
constexpr uint8_t DEVID_64M = 0x16;

constexpr uint32_t cs_deselect_time_ns = 50;
constexpr uint32_t cs_active_setup_time_relative_to_clk_ns = 5;
constexpr uint32_t tSUS_ns = 20000;

/**
 * To avoid deadlock erase_mutex must be always acquired
 * earlier than communication_mutex and communication_mutex
 * must be always released earlier than erase_mutex.
 *
 * Chip erase operation:
 * erase_mutex must be acquired first then
 * communication_mutex must be acquired
 * Do whole chip erase operation (can not be suspended)
 * communication_mutex must be released first
 * erase_mutex must be released after communication_mutex is released
 *
 * Block erase operation:
 * erase_mutex must be acquired first then
 * communication_mutex must be acquired
 * is_erasing must be set to true
 * start erase operation
 * communication_mutex must be release first
 * communication_mutex must be acquired
 * if erase operation finished is_erasing must be set to false
 * communication_mutex must be released first
 * erase_mutex must be released after communication_mutex is released
 *
 * Read/write operation standard priority (for each single block in case of write):
 * erase_mutex must be acquired first then
 * communication_mutex must be acquired
 * do read/write operation
 * communication_mutex must be released first
 * erase_mutex must be released after communication_mutex is released
 *
 * Write operation high priority for multiple blocks at once:
 * Try to acquire erase_mutex
 * acquire communication_mutex and check is_erasing.
 * If is_erasing pause erase operation
 * do read/write operation
 * If is_erasing resume erase operation
 * communication_mutex must be released first
 * erase_mutex (if acquired) must be released after communication_mutex is released
 *
 * Read operation high priority not implemented / not needed yet.
 *@{
 */
osMutexDef(erase_mutex_resource);
osMutexId erase_mutex = NULL;

osMutexDef(communication_mutex_resource);
osMutexId communication_mutex = NULL;

/**@}*/

/**
 * @brief Block erase operation is in progress
 *
 * Block erase operation needs to be paused if read or write is needed immediately.
 *
 * This variable can be modified only when both erase_mutex and communication_mutex
 * are acquired.
 *
 * This variable can be read only when communication_mutex is acquired.
 *
 * It needs to be checked only when erase_mutex was not acquired successfully, otherwise
 * holding erase_mutex is sufficient to be sure erasing is not in progress.
 *
 * This variable exist because both erase_mutex and communication_mutex must be held
 * to start erase operation. So erase implementation sets this to true once it acquires both
 * mutexes. This ensures that high priority read or write operation doesn't assume erase
 * is already ongoing if it fails to acquire erase_mutex and acquires communication_mutex
 * in the moment erase acquired erase_mutex and didn't acquired communication_mutex yet for
 * the first time to start erase operation.
 *
 */
bool is_erasing_block = false;
uint8_t device_id;

constexpr uint32_t max_wait_loops() {
    constexpr uint32_t max_bitrate_hz = 133000000;
    constexpr uint32_t bits_per_status_read = 16;
    constexpr uint64_t nanoseconds_per_second = 1000000000ull;
    constexpr uint32_t transfer_duration_ns = bits_per_status_read * nanoseconds_per_second / max_bitrate_hz;
    constexpr uint32_t cs_duration_ns = cs_deselect_time_ns + cs_active_setup_time_relative_to_clk_ns;

    constexpr uint32_t max_operation_duration_s = 100; // W25Q64JV Chip erase
    return (nanoseconds_per_second / (transfer_duration_ns + cs_duration_ns) * max_operation_duration_s);
}

void w25x_select() {
    buddy::hw::extFlashCs.write(buddy::hw::Pin::State::low);
    // Currently less than 1 CPU cycle, so no delay is done
    delay_ns_precise<cs_active_setup_time_relative_to_clk_ns>();
}

void w25x_deselect() {
    buddy::hw::extFlashCs.write(buddy::hw::Pin::State::high);
    delay_ns_precise<cs_deselect_time_ns>();
}

void write_enable(void) {
    w25x_select();
    w25x_send_byte(CMD_ENABLE_WR); // send command 0x06
    w25x_deselect();
}

w25x_status1_t read_status1_reg() {
    w25x_select();
    w25x_send_byte(CMD_RD_STATUS1_REG);
    w25x_status1_t status = static_cast<w25x_status1_t>(w25x_receive_byte());
    w25x_deselect();
    return status;
}

w25x_status2_t read_status2_reg() {
    w25x_select();
    w25x_send_byte(CMD_RD_STATUS2_REG);
    w25x_status2_t status = static_cast<w25x_status2_t>(w25x_receive_byte());
    w25x_deselect();
    return status;
}

bool is_suspended() {
    return (read_status2_reg() & W25X_STATUS2_SUS);
}

bool w25x_wait_busy(void) {
    uint32_t loop_counter = 0;
    while (read_status1_reg() & W25X_STATUS_BUSY) {
        ++loop_counter;
        if (loop_counter > max_wait_loops()) {
            return false;
        }
    }
    return true;
}

bool w25x_wait_erase(void) {
    w25x_status1_t status;
    uint32_t loop_counter = 0;

    do {
        OptionalMutex communicationMutex(communication_mutex);
        ++loop_counter;
        status = read_status1_reg();
        if (!(status & W25X_STATUS_BUSY)) {
            is_erasing_block = false;
        }
        if (loop_counter > max_wait_loops()) {
            return false;
        }
    } while (status & W25X_STATUS_BUSY);

    return true;
}

CmdWithAddress cmd_with_address(uint8_t cmd, uint32_t addr) {
    CmdWithAddress cmdWithAddress = { { cmd,
        reinterpret_cast<uint8_t *>(&addr)[2],
        reinterpret_cast<uint8_t *>(&addr)[1],
        reinterpret_cast<uint8_t *>(&addr)[0] } };
    return cmdWithAddress;
}

/**
 *
 * @param addr
 * @param data
 * @param cnt
 * @param high_priority
 *  @n true Do not lock mutexes (already done by w25x_program())
 *  @n false Lock mutexes when accessing w25x
 */
void program_page(uint32_t addr, const uint8_t *data, uint16_t cnt, bool high_priority) {
    OptionalMutex eraseMutex(high_priority ? NULL : erase_mutex);
    OptionalMutex communicationMutex(high_priority ? NULL : communication_mutex);

    write_enable();
    w25x_select();
    CmdWithAddress cmdWithAddress = cmd_with_address(CMD_PAGE_PROGRAM, addr);
    w25x_send(cmdWithAddress.buffer, sizeof(cmdWithAddress.buffer));
    w25x_send(data, cnt);
    w25x_deselect();
    if (!w25x_wait_busy()) {
        w25x_set_error(HAL_TIMEOUT);
    }
}

void split_page_program(uint32_t addr, const uint8_t *data, uint32_t cnt, bool high_priority) {
    // Write unaligned part first
    uint32_t addr_align = addr % PAGE_SIZE;
    if (addr_align != 0) {
        uint32_t cnt_align = PAGE_SIZE - addr_align;
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
    while (cnt >= PAGE_SIZE) {
        program_page(addr, data, PAGE_SIZE, high_priority);
        addr += PAGE_SIZE;
        data += PAGE_SIZE;
        cnt -= PAGE_SIZE;
    }

    // Write the remaining data
    if (cnt > 0) {
        program_page(addr, data, cnt, high_priority);
    }
}

void suspend_erase() {
    w25x_select();
    w25x_send_byte(CMD_ERASE_PROGRAM_SUSPEND);
    w25x_deselect();
    // W25Q guarantees to be available in tSUS
    // alternatively busy status can be polled
    delay_ns_precise<tSUS_ns>();
}

void resume_erase() {
    w25x_select();
    w25x_send_byte(CMD_ERASE_PROGRAM_RESUME);
    w25x_deselect();
    // Assure that suspend is not called earlier than in tSUS
    // after resume
    delay_ns_precise<tSUS_ns>();
}

void w25x_erase(uint8_t cmd, uint32_t addr) {
    OptionalMutex eraseMutex(erase_mutex);
    {
        OptionalMutex communicationMutex(communication_mutex);
        is_erasing_block = true;
        write_enable();
        w25x_select();
        CmdWithAddress cmdWithAddress = cmd_with_address(cmd, addr);
        w25x_send(cmdWithAddress.buffer, sizeof(cmdWithAddress.buffer));
        w25x_deselect();
    }
    if (!w25x_wait_erase()) {
        w25x_set_error(HAL_TIMEOUT);
    }
}

int mfrid_devid(uint8_t *devid) {
    w25x_select();
    CmdWithAddress cmdWithAddress = cmd_with_address(CMD_MFRID_DEVID, 0ul);
    w25x_send(cmdWithAddress.buffer, sizeof(cmdWithAddress.buffer));
    uint8_t w25x_mfrid __attribute__((unused)) = w25x_receive_byte(); // receive mfrid
    uint8_t w25x_devid = w25x_receive_byte();                         // receive devid
    w25x_deselect();
    if (devid) {
        *devid = w25x_devid;
    }
    return ((w25x_devid == DEVID_8M) || (w25x_devid == DEVID_16M) || (w25x_devid == DEVID_32M) || (w25x_devid == DEVID_64M));
}

} // end anonymous namespace

bool w25x_init() {
    const bool os_running = xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;

    erase_mutex = os_running ? osMutexCreate(osMutex(erase_mutex_resource)) : NULL;
    communication_mutex = os_running ? osMutexCreate(osMutex(communication_mutex_resource)) : NULL;

    static_assert(!NULL, "All the code expects NULL in condition to evaluate as false.");
    if (os_running && (!erase_mutex || !communication_mutex)) {
        return false;
    }

    OptionalMutex eraseMutex(erase_mutex);
    OptionalMutex communicationMutex(communication_mutex);

    if (!w25x_communication_abort()) {
        return false;
    }

    w25x_deselect();

    if (!w25x_communication_init(os_running)) {
        return false;
    }

    if (!w25x_wait_busy()) {
        return false;
    }

    if (is_suspended()) {
        resume_erase();
        if (!w25x_wait_busy()) {
            return false;
        }
    }

    is_erasing_block = false;

    if (!mfrid_devid(&device_id)) {
        return false;
    }

    return true;
}

uint32_t w25x_get_sector_count() {
    if (device_id == DEVID_8M) {
        return 256;
    } else if (device_id == DEVID_16M) {
        return 512;
    } else if (device_id == DEVID_32M) {
        return 1024;
    } else if (device_id == DEVID_64M) {
        return 2048;
    } else {
        abort();
    }
}

void w25x_rd_data(uint32_t addr, uint8_t *data, uint16_t cnt) {
    OptionalMutex eraseMutex(erase_mutex);
    OptionalMutex communicationMutex(communication_mutex);

    w25x_select();
    CmdWithAddress cmdWithAddress = cmd_with_address(CMD_RD_DATA, addr);
    w25x_send(cmdWithAddress.buffer, sizeof(cmdWithAddress.buffer));
    w25x_receive(data, cnt);
    w25x_deselect();
}

void w25x_program(uint32_t addr, const uint8_t *data, uint32_t cnt) {
    // For high priority address range block erase can be suspended
    // and w25x is locked until all pages are written in once and then
    // suspended erase operation is resumed.
    // For low priority address range w25x is locked only on per page basis
    // so higher priority task can write / erase in between lower priority task
    // w25x_program call is split into single page write.
    const bool high_priority = (addr >= w25x_pp_start_address) && (addr < w25x_fs_start_address);
    const bool wait = !high_priority;

    OptionalMutex eraseMutex(high_priority ? erase_mutex : NULL, wait);
    OptionalMutex communicationMutex(high_priority ? communication_mutex : NULL);

    if (is_erasing_block) {
        suspend_erase();
    }
    split_page_program(addr, data, cnt, high_priority);

    if (is_erasing_block) {
        resume_erase();
    }
}

void w25x_sector_erase(uint32_t addr) {
    w25x_erase(CMD_SECTOR_ERASE, addr);
}

void w25x_block32_erase(uint32_t addr) {
    w25x_erase(CMD_BLOCK32_ERASE, addr);
}

void w25x_block64_erase(uint32_t addr) {
    w25x_erase(CMD_BLOCK64_ERASE, addr);
}

void w25x_chip_erase(void) {
    OptionalMutex eraseMutex(erase_mutex);
    OptionalMutex communicationMutex(communication_mutex);

    write_enable();
    w25x_select();
    w25x_send_byte(CMD_CHIP_ERASE); // send command 0xc7
    w25x_deselect();
    if (!w25x_wait_busy()) {
        w25x_set_error(HAL_TIMEOUT);
    }
}

#if 0 // unused
void w25x_rd_uid(uint8_t *uid) {
    OptionalMutex eraseMutex(erase_mutex);
    {
        OptionalMutex communicationMutex(communication_mutex);
        w25x_select();
        w25x_send_byte(CMD_RD_UID); // send command 0x4b
        uint8_t cnt = 4;            // 4 dummy bytes
        while (cnt--)               // receive dummy bytes
            w25x_receive_byte();
        cnt = 8;      // 8 bytes UID
        while (cnt--) // receive UID
            uid[7 - cnt] = w25x_receive_byte();
        w25x_deselect();
    }
}
#endif
