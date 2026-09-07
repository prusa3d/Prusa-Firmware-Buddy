/// @file
#include <common/st25dv64k.h>

#include <algorithm>
#include <common/bsod.h>
#include <common/i2c.hpp>
#include <common/st25dv64k_internal.h>
#include <common/timing.h>
#include <cstring>
#include <error_codes.hpp>
#include <freertos/mutex.hpp>
#include <utility>

namespace {
constexpr const uint8_t BLOCK_DELAY = 5; // block delay [ms]
constexpr const uint8_t BLOCK_BYTES = 4; // bytes per block
constexpr const uint16_t SEQUENTIAL_WRITE_BYTES = 256; // DS10925 6.4.2: max bytes per I2C sequential write

constexpr const uint32_t RETRIES = 3;

freertos::Mutex &st25dv64k_mutex() {
    // Has to be initialized lazily - global variables get initialized after the EEPROM is first used
    static freertos::Mutex r;
    return r;
}

inline void st25dv64k_lock() {
    st25dv64k_mutex().lock();
}

inline void st25dv64k_unlock() {
    st25dv64k_mutex().unlock();
}

// For some reason, things go wrong if you try to use osDelay
#define st25dv64k_delay HAL_Delay

constexpr uint16_t page_address(uint16_t address) {
    return address & ~(BLOCK_BYTES - 1);
}

constexpr uint16_t page_offset(uint16_t address) {
    return address & (BLOCK_BYTES - 1);
}

constexpr bool is_page_aligned(uint16_t address) {
    return page_offset(address) == 0;
}

uint8_t dirty_data[BLOCK_BYTES]; ///< page-sized buffer for dirty bytes
uint16_t dirty_address = 0; ///< address of the first dirty byte
uint8_t dirty_size = 0; ///< number of dirty bytes

// DS10925 Rev 11, 6.4.3 Minimizing system delays by polling on ACK
// Datasheet says that although maximum write time is 5 ms for 4-byte page write up to 85 °C,
// typical write time is faster and it recommends using ACK-polling.
[[nodiscard]] i2c::Result wait_write_cycle(uint16_t size) {
    const uint32_t deadline = ticks_ms() + BLOCK_DELAY * size / BLOCK_BYTES + BLOCK_DELAY;
    for (;;) {
        const uint32_t trials = 1;
        const uint32_t timeout = 1;
        const i2c::Result result = i2c::IsDeviceReady(*i2c_handle_eeprom, std::to_underlying(EepromCommandWrite::addr_memory), trials, timeout);
        if (result == i2c::Result::ok) {
            return result; // device is ready
        }
        if (ticks_diff(deadline, ticks_ms()) < 0) {
            return result; // prevent infinite loop and allow recovery when device or bus is stuck
        }
    }
}

/// Read the memory after write and verify it was written.
/// Chip itself works correctly, but we don't trust the bus.
[[nodiscard]] i2c::Result verify_memory(uint16_t address, const uint8_t *src, uint16_t size) {
    uint8_t buffer[32];
    for (uint16_t pos = 0; pos < size; pos += sizeof(buffer)) {
        const uint16_t chunk = std::min<uint16_t>(sizeof(buffer), size - pos);
        const i2c::Result result = i2c::Mem_Read_16bit_Addr(*i2c_handle_eeprom, std::to_underlying(EepromCommandRead::addr_memory), address + pos, buffer, chunk, HAL_MAX_DELAY);
        if (result != i2c::Result::ok) {
            return result;
        }
        if (memcmp(buffer, src + pos, chunk) != 0) {
            return i2c::Result::error;
        }
    }
    return i2c::Result::ok;
}

/// Helper to write memory, wait for completion and verify write was correct.
[[nodiscard]] i2c::Result mem_write_verified(uint16_t address, uint8_t *data, uint16_t size) {
    debug_assert(size > 0);
    // Large blocks are always aligned, small ones are always buffered
    debug_assert(is_page_aligned(address) ? (size <= SEQUENTIAL_WRITE_BYTES) : page_address(address) == page_address(address + size - 1));
    if (const i2c::Result result = i2c::Mem_Write_16bit_Addr(*i2c_handle_eeprom, std::to_underlying(EepromCommandWrite::addr_memory), address, data, size, HAL_MAX_DELAY); result != i2c::Result::ok) {
        return result;
    }
    if (const i2c::Result result = wait_write_cycle(size); result != i2c::Result::ok) {
        return result;
    }
    return verify_memory(address, data, size);
}

/// Helper to write memory, wait for completion and verify write was correct,
/// with retries and recovery.
[[nodiscard]] i2c::Result mem_write_verified_retried(uint16_t address, uint8_t *data, uint16_t size) {
    i2c::Result result = i2c::Result::error;
    for (uint32_t try_no = 0; try_no < RETRIES; ++try_no) {
        result = mem_write_verified(address, data, size);
        if (result == i2c::Result::ok) {
            break;
        }
        try_fix_if_needed(result);
    }
    return result;
}

[[nodiscard]] i2c::Result st25dv64k_user_write_bytes_flush_internal() {
    if (dirty_size == 0) {
        return i2c::Result::ok;
    }

    const i2c::Result result = mem_write_verified_retried(dirty_address, dirty_data, dirty_size);
    if (result == i2c::Result::ok) {
        dirty_size = 0; // commit
    }
    return result;
}

[[nodiscard]] i2c::Result st25dv64k_user_write_bytes_buffered_internal(uint16_t address, const void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    const uint8_t *src = static_cast<const uint8_t *>(pdata);

    // A buffered partial page that isn't contiguous with this write is stale - commit it first.
    if (dirty_size > 0
        && address != dirty_address + dirty_size) {
        if (const i2c::Result result = st25dv64k_user_write_bytes_flush_internal(); result != i2c::Result::ok) {
            return result;
        }
    }

    // Head: append into the buffered/partial page; flush once it fills, otherwise keep buffering.
    if (dirty_size > 0 || !is_page_aligned(address)) {
        if (dirty_size == 0) {
            dirty_address = address;
        }
        debug_assert(address == dirty_address + dirty_size);

        const uint16_t chunk = std::min<uint16_t>(size, BLOCK_BYTES - page_offset(address));
        memcpy(&dirty_data[dirty_size], src, chunk);
        dirty_size += chunk;
        src += chunk;
        address += chunk;
        size -= chunk;

        if (!is_page_aligned(address)) {
            return i2c::Result::ok; // page not full yet, keep buffering
        }
        if (const i2c::Result result = st25dv64k_user_write_bytes_flush_internal(); result != i2c::Result::ok) {
            return result;
        }
    }

    // Address is now page-aligned and the buffer is empty.
    debug_assert(is_page_aligned(address));

    // Bulk: write whole pages via the fast multi-page frame, with verify + retry.
    while (size >= BLOCK_BYTES) {
        const uint16_t chunk = std::min<uint16_t>(SEQUENTIAL_WRITE_BYTES, size) & ~(BLOCK_BYTES - 1);
        if (const i2c::Result result = mem_write_verified_retried(address, const_cast<uint8_t *>(src), chunk); result != i2c::Result::ok) {
            return result;
        }
        src += chunk;
        address += chunk;
        size -= chunk;
    }

    // Tail: keep the sub-page remainder buffered to coalesce with the next write.
    if (size > 0) {
        dirty_address = address;
        dirty_size = size;
        memcpy(dirty_data, src, size);
    }
    return i2c::Result::ok;
}

} // namespace

void rise_error_if_needed(i2c::Result result) {
    switch (result) {
    case i2c::Result::ok:
        break;
    case i2c::Result::busy_after_retries:
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_BUSY);
        break;
    case i2c::Result::error:
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_ERROR);
        break;
    case i2c::Result::timeout:
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_TIMEOUT);
        break;
    }
}

void try_fix_if_needed(const i2c::Result &result) {
    switch (result) {
    case i2c::Result::busy_after_retries:
    case i2c::Result::error:
        i2c_init_eeprom();
        [[fallthrough]];
    case i2c::Result::timeout:
    case i2c::Result::ok:
        break;
    }
}

[[nodiscard]] i2c::Result eeprom_transmit(EepromCommandWrite cmd, uint8_t *pData, uint16_t size) {
    return i2c::Transmit(*i2c_handle_eeprom, std::to_underlying(cmd), pData, size, HAL_MAX_DELAY);
}

[[nodiscard]] i2c::Result user_write_address_without_lock(EepromCommandWrite cmd, uint16_t address) {
    uint8_t _out[sizeof(address)];
    _out[0] = address >> 8;
    _out[1] = address & 0xff;

    i2c::Result result = eeprom_transmit(cmd, _out, sizeof(address));
    st25dv64k_delay(BLOCK_DELAY);

    return result;
}

[[nodiscard]] i2c::Result user_write_bytes_without_lock(EepromCommandWrite cmd, uint16_t address, const void *pdata, uint16_t size) {
    if (size == 0 || pdata == nullptr) {
        return user_write_address_without_lock(cmd, address);
    }

    uint8_t const *p = (uint8_t const *)pdata;
    uint8_t _out[sizeof(address) + BLOCK_BYTES];
    while (size) {
        uint8_t block_size = BLOCK_BYTES - (address % BLOCK_BYTES);
        if (block_size > size) {
            block_size = size;
        }
        _out[0] = address >> 8;
        _out[1] = address & 0xff;
        memcpy(_out + sizeof(address), p, block_size);

        i2c::Result result = eeprom_transmit(cmd, _out, sizeof(address) + block_size);
        if (result != i2c::Result::ok) {
            return result;
        }

        st25dv64k_delay(BLOCK_DELAY);

        size -= block_size;
        address += block_size;
        p += block_size;
    }
    return i2c::Result::ok;
}

[[nodiscard]] i2c::Result user_read_bytes_without_lock(EepromCommand cmd, uint16_t address, void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = user_write_address_without_lock(eeprom_get_write_address(cmd), address);
    if (result == i2c::Result::ok) {
        result = i2c::Receive(*i2c_handle_eeprom, std::to_underlying(eeprom_get_read_address(cmd)), static_cast<uint8_t *>(pdata), size, HAL_MAX_DELAY);
    }

    return result;
}

[[nodiscard]] i2c::Result user_read_bytes(EepromCommand cmd, uint16_t address, void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = i2c::Result::error;

    // receive retry requires new transmit
    for (uint32_t try_no = 0; (result != i2c::Result::ok) && (try_no < RETRIES); ++try_no) {
        st25dv64k_lock();
        result = user_read_bytes_without_lock(cmd, address, pdata, size);
        st25dv64k_unlock();
        try_fix_if_needed(result);
    }

    return result;
}

[[nodiscard]] i2c::Result user_write_bytes(EepromCommand cmd, uint16_t address, const void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = i2c::Result::error;
    bool match = false;

    for (uint32_t try_no = 0; try_no < RETRIES; ++try_no) {

        st25dv64k_lock();
        result = user_write_bytes_without_lock(EepromCommandWrite::addr_memory, address, pdata, size);
        if (result == i2c::Result::ok) {
            // Verify the data being written correctly
            uint8_t chunk_read[32];
            uint16_t read_pos = 0;
            match = true;
            while (read_pos < size) {
                uint16_t to_read = std::min<uint16_t>(sizeof(chunk_read), size - read_pos);
                result = user_read_bytes_without_lock(cmd, address + read_pos, chunk_read, to_read);
                if (result != i2c::Result::ok) {
                    break;
                }
                if (memcmp(chunk_read, ((uint8_t *)pdata) + read_pos, to_read)) {
                    match = false;
                    break;
                }
                read_pos += to_read;
            }
        }
        st25dv64k_unlock();
        try_fix_if_needed(result);

        match = match && result == i2c::Result::ok;
        // Stop retrying on match
        if (match) {
            break;
        }
    }

    return result;
}

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size) {
    st25dv64k_user_write_bytes_flush(); // commit pending buffered writes so the read sees them
    auto result = user_read_bytes(EepromCommand::memory, address, pdata, size);
    rise_error_if_needed(result);
}

void st25dv64k_user_write_bytes(uint16_t address, const void *pdata, uint16_t size) {
    st25dv64k_lock();
    rise_error_if_needed(st25dv64k_user_write_bytes_buffered_internal(address, pdata, size));
    rise_error_if_needed(st25dv64k_user_write_bytes_flush_internal());
    st25dv64k_unlock();
}

uint8_t st25dv64k_rd_cfg(uint16_t address) {
    uint8_t data;
    auto result = user_read_bytes(EepromCommand::registers, address, &data, sizeof(data));
    rise_error_if_needed(result);
    return data;
}

void st25dv64k_wr_cfg(uint16_t address, uint8_t data) {
    auto result = user_write_bytes(EepromCommand::registers, address, &data, sizeof(data));
    rise_error_if_needed(result);
}

void st25dv64k_present_pwd(uint8_t *pwd) {
    uint8_t _out[19] = { 0x09, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0x09, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (pwd) {
        memcpy(_out + 2, pwd, 8);
        memcpy(_out + 11, pwd, 8);
    }

    i2c::Result result = i2c::Result::error;

    for (uint32_t try_no = 0; (result != i2c::Result::ok) && (try_no < RETRIES); ++try_no) {
        st25dv64k_lock();
        result = eeprom_transmit(EepromCommandWrite::addr_registers, _out, sizeof(_out));
        st25dv64k_unlock();
        try_fix_if_needed(result);
    }

    rise_error_if_needed(result);
}

void st25dv64k_init() {
    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA3, 0xFF);
    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA2, 0xFF); // AREA2 0x500 to end
    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA1, 0x27); // AREA1 0 to 0x4ff

    st25dv64k_wr_cfg(REG_RFA1SS, 0b0); // AREA 1 RF R/W
    st25dv64k_wr_cfg(REG_RFA2SS, 0b1101); // AREA 2 RF N/A
    st25dv64k_wr_cfg(REG_RFA3SS, 0b1101); // AREA 3 RF N/A
}

void st25dv64k_user_write_bytes_buffered(uint16_t address, const void *pdata, uint16_t size) {
    st25dv64k_lock();
    rise_error_if_needed(st25dv64k_user_write_bytes_buffered_internal(address, pdata, size));
    st25dv64k_unlock();
}

void st25dv64k_user_write_bytes_flush() {
    st25dv64k_lock();
    rise_error_if_needed(st25dv64k_user_write_bytes_flush_internal());
    st25dv64k_unlock();
}
