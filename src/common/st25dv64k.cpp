#include "st25dv64k.h"
#include "st25dv64k_internal.h"

#include <option/has_nfc.h>
#include <nfc.hpp>

#include "i2c.hpp"
#include "cmsis_os.h"
#include "bsod.h"
#include "utility_extensions.hpp"

#include <string.h>
#include <algorithm>

namespace {

#define ST25DV64K_RTOS

constexpr const uint8_t BLOCK_DELAY = 5; // block delay [ms]
constexpr const uint8_t BLOCK_BYTES = 4; // bytes per block

constexpr const uint32_t RETRIES = 3;

#define DELAY HAL_Delay

uint8_t st25dv64k_initialised = 0;

#ifdef ST25DV64K_RTOS

    #include "cmsis_os.h"

osSemaphoreId st25dv64k_sema = 0; // semaphore handle

inline void st25dv64k_lock() {
    if (st25dv64k_sema == 0) {
        osSemaphoreDef(st25dv64kSema);
        st25dv64k_sema = osSemaphoreCreate(osSemaphore(st25dv64kSema), 1);
    }
    osSemaphoreWait(st25dv64k_sema, osWaitForever);
}

inline void st25dv64k_unlock() {
    osSemaphoreRelease(st25dv64k_sema);
}

    #define st25dv64k_delay osDelay

#else
    #define st25dv64k_lock()
    #define st25dv64k_unlock()
    #define st25dv64k_delay HAL_Delay
#endif // ST25DV64K_RTOS

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
        I2C_INIT(eeprom);
        [[fallthrough]];
    case i2c::Result::timeout:
    case i2c::Result::ok:
        break;
    }
}

[[nodiscard]] i2c::Result eeprom_transmit(EepromCommandWrite cmd, uint8_t *pData, uint16_t size) {
    return i2c::Transmit(I2C_HANDLE_FOR(eeprom), ftrstd::to_underlying(cmd), pData, size, HAL_MAX_DELAY);
}

[[nodiscard]] i2c::Result user_write_address_without_lock(EepromCommandWrite cmd, uint16_t address) {
    uint8_t _out[sizeof(address)];
    _out[0] = address >> 8;
    _out[1] = address & 0xff;

    i2c::Result result = eeprom_transmit(cmd, _out, sizeof(address));
    DELAY(BLOCK_DELAY);

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

        DELAY(BLOCK_DELAY);

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
        result = i2c::Receive(I2C_HANDLE_FOR(eeprom), ftrstd::to_underlying(eeprom_get_read_address(cmd)), static_cast<uint8_t *>(pdata), size, HAL_MAX_DELAY);
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

[[nodiscard]] i2c::Result user_unverified_write_bytes(uint16_t address, const void *pdata, uint16_t size) {
    if (size == 0) {
        return i2c::Result::ok;
    }

    i2c::Result result = i2c::Result::error;

    for (uint32_t try_no = 0; try_no < RETRIES; ++try_no) {

        st25dv64k_lock();
        result = user_write_bytes_without_lock(EepromCommandWrite::addr_memory, address, pdata, size);
        st25dv64k_unlock();
        try_fix_if_needed(result);

        if (result == i2c::Result::ok) {
            break;
        }
    }

    return result;
}

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size) {
    auto result = user_read_bytes(EepromCommand::memory, address, pdata, size);
    rise_error_if_needed(result);
}

uint8_t st25dv64k_user_read(uint16_t address) {
    uint8_t data;
    st25dv64k_user_read_bytes(address, &data, sizeof(data));
    return data;
}

void st25dv64k_user_write_bytes(uint16_t address, const void *pdata, uint16_t size) {
    auto result = user_write_bytes(EepromCommand::memory, address, pdata, size);
    rise_error_if_needed(result);
}

void st25dv64k_user_unverified_write_bytes(uint16_t address, const void *pdata, uint16_t size) {
    auto result = user_unverified_write_bytes(address, pdata, size);
    rise_error_if_needed(result);
}

void st25dv64k_user_write(uint16_t address, uint8_t data) {
    st25dv64k_user_write_bytes(address, &data, sizeof(data));
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
    if (st25dv64k_initialised) {
        return;
    }
    st25dv64k_initialised = 1;

    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA3, 0xFF);
    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA2, 0xFF); // AREA2 0x500 to end
    st25dv64k_present_pwd(0);
    st25dv64k_wr_cfg(REG_ENDA1, 0x27); // AREA1 0 to 0x4ff

    st25dv64k_wr_cfg(REG_RFA1SS, 0b0); // AREA 1 RF R/W
    st25dv64k_wr_cfg(REG_RFA2SS, 0b1101); // AREA 2 RF N/A
    st25dv64k_wr_cfg(REG_RFA3SS, 0b1101); // AREA 3 RF N/A

#if HAS_NFC()
    nfc::init();
#endif
}
