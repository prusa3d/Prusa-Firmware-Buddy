// st25dv64k.c

#include "st25dv64k.h"
#include "i2c.h"
#include <string.h>
#include "cmsis_os.h"
#include "main.h"
#include "metric.h"

#define ST25DV64K_RTOS

//system config address registr
// static const uint16_t REG_GPO = 0x0000;
// static const uint16_t REG_IT_TIME = 0x0001;
// static const uint16_t REG_EH_MODE = 0x0002;
// static const uint16_t REG_RF_MNGT = 0x0003;
// static const uint16_t REG_RFA1SS = 0x0004;
static const uint16_t REG_ENDA1 = 0x0005;
// static const uint16_t REG_RFA2SS = 0x0006;
static const uint16_t REG_ENDA2 = 0x0007;
// static const uint16_t REG_RFA3SS = 0x0008;
static const uint16_t REG_ENDA3 = 0x0009;
// static const uint16_t REG_RFA4SS = 0x000A;
// static const uint16_t REG_I2CSS = 0x000B;
// static const uint16_t REG_LOCK_CCFILE = 0x000C;
// static const uint16_t REG_MB_MODE = 0x000D;
// static const uint16_t REG_MB_WDG = 0x000E;
// static const uint16_t REG_LOCK_CFG = 0x000F;

// EEPROM I2C addresses
static const uint16_t ADDR_WRITE = 0xA6;
static const uint16_t ADDR_READ = 0xA7;
static const uint16_t ADDR_WRITE_SYS = 0xAE;
static const uint16_t ADDR_READ_SYS = 0xAF;

static const uint8_t BLOCK_DELAY = 5; // block delay [ms]
static const uint8_t BLOCK_BYTES = 4; // bytes per block

static const uint8_t WRITE_RETRIES = 3;

#define DELAY HAL_Delay

uint8_t st25dv64k_initialised = 0;

#ifdef ST25DV64K_RTOS

    #include "cmsis_os.h"
osSemaphoreId st25dv64k_sema = 0; // semaphore handle

static inline void st25dv64k_lock(void) {
    if (st25dv64k_sema == 0) {
        osSemaphoreDef(st25dv64kSema);
        st25dv64k_sema = osSemaphoreCreate(osSemaphore(st25dv64kSema), 1);
    }
    osSemaphoreWait(st25dv64k_sema, osWaitForever);
}

static inline void st25dv64k_unlock(void) {
    osSemaphoreRelease(st25dv64k_sema);
}

    #define st25dv64k_delay osDelay

#else

    #define st25dv64k_lock()

    #define st25dv64k_unlock()

    #define st25dv64k_delay HAL_Delay

#endif //ST25DV64K_RTOS

void st25dv64k_init(void) {
    uint8_t pwd[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    if (!st25dv64k_initialised) {
        if ((st25dv64k_rd_cfg(REG_ENDA1) != 0x7f)
            && (st25dv64k_rd_cfg(REG_ENDA2) != 0xff)
            && (st25dv64k_rd_cfg(REG_ENDA3) != 0xff)) {
            st25dv64k_present_pwd(0);
            st25dv64k_wr_cfg(REG_ENDA3, 0xFF);
            st25dv64k_wr_cfg(REG_ENDA2, 0xFF);
            st25dv64k_wr_cfg(REG_ENDA1, 0x7F);
            st25dv64k_present_pwd(pwd);

            st25dv64k_rd_cfg(REG_ENDA1);
            st25dv64k_rd_cfg(REG_ENDA2);
            st25dv64k_rd_cfg(REG_ENDA3);
        }
        st25dv64k_initialised = 1;
    }
}

uint8_t st25dv64k_user_read(uint16_t address) {
    uint8_t _out[2] = { static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address & 0xff) };
    uint8_t data;
    st25dv64k_lock();
    I2C_Transmit(&I2C_HANDLE_FOR(eeprom), ADDR_WRITE, _out, 2, HAL_MAX_DELAY);
    I2C_Receive(&I2C_HANDLE_FOR(eeprom), ADDR_READ, &data, 1, HAL_MAX_DELAY);
    st25dv64k_unlock();
    return data;
}

static void st25dv64k_user_write_unchecked(uint16_t address, uint8_t data) {
    uint8_t _out[3] = { static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address & 0xff), data };
    st25dv64k_lock();
    I2C_Transmit(&I2C_HANDLE_FOR(eeprom), ADDR_WRITE, _out, 3, HAL_MAX_DELAY);
    DELAY(BLOCK_DELAY);
    st25dv64k_unlock(); // unlock must be here because other threads cannot access eeprom while writing/waiting
}

void st25dv64k_user_write(uint16_t address, uint8_t data) {
    for (uint8_t i = 0; i < WRITE_RETRIES; ++i) {
        st25dv64k_user_write_unchecked(address, data);
        const uint8_t data_read = st25dv64k_user_read(address);
        if (data_read == data) {
            break;
        }
    }
}

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size) {
    uint8_t _out[2] = { static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address & 0xff) };
    st25dv64k_lock();
    I2C_Transmit(&I2C_HANDLE_FOR(eeprom), ADDR_WRITE, _out, 2, HAL_MAX_DELAY);
    I2C_Receive(&I2C_HANDLE_FOR(eeprom), ADDR_READ, static_cast<uint8_t *>(pdata), size, HAL_MAX_DELAY);
    st25dv64k_unlock();
}

static void st25dv64k_user_write_bytes_unchecked(uint16_t address, void const *pdata, uint16_t size) {
    static metric_t metric_eeprom_write = METRIC("eeprom_write", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_ENABLE_ALL);
    metric_record_event(&metric_eeprom_write);
    uint8_t const *p = (uint8_t const *)pdata;
    uint8_t _out[6];
    uint8_t block_size;
    st25dv64k_lock();
    while (size) {
        block_size = BLOCK_BYTES - (address % BLOCK_BYTES);
        if (block_size > size)
            block_size = size;
        _out[0] = address >> 8;
        _out[1] = address & 0xff;
        memcpy(_out + 2, p, block_size);
        I2C_Transmit(&I2C_HANDLE_FOR(eeprom), ADDR_WRITE, _out, 2 + block_size, HAL_MAX_DELAY);
        DELAY(BLOCK_DELAY);
        size -= block_size;
        address += block_size;
        p += block_size;
    }
    st25dv64k_unlock(); // unlock must be here because other threads cannot access eeprom while writing/waiting
}

static inline uint16_t min(const uint16_t a, const uint16_t b) {
    return a < b ? a : b;
}

void st25dv64k_user_write_bytes(uint16_t address, void const *pdata, uint16_t size) {
    for (uint8_t i = 0; i < WRITE_RETRIES; ++i) {
        st25dv64k_user_write_bytes_unchecked(address, pdata, size);

        // Verify the data being written correctly
        uint8_t chunk_read[32];
        uint16_t read_pos = 0;
        bool match = true;
        while (read_pos < size) {
            uint16_t to_read = min(sizeof(chunk_read), size - read_pos);
            st25dv64k_user_read_bytes(address + read_pos, chunk_read, to_read);
            if (memcmp(chunk_read, ((uint8_t *)pdata) + read_pos, to_read)) {
                match = false;
            }
            read_pos += to_read;
        }

        // Stop retrying on match
        if (match) {
            break;
        }
    }
}

uint8_t st25dv64k_rd_cfg(uint16_t address) {
    uint8_t _out[2] = { static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address & 0xff) };
    uint8_t data;
    st25dv64k_lock();
    I2C_Transmit(&I2C_HANDLE_FOR(eeprom), ADDR_WRITE_SYS, _out, 2, HAL_MAX_DELAY);
    I2C_Receive(&I2C_HANDLE_FOR(eeprom), ADDR_READ_SYS, &data, 1, HAL_MAX_DELAY);
    st25dv64k_unlock();
    return data;
}

void st25dv64k_wr_cfg(uint16_t address, uint8_t data) {
    uint8_t _out[3] = { static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address & 0xff), data };
    st25dv64k_lock();
    I2C_Transmit(&I2C_HANDLE_FOR(eeprom), ADDR_WRITE_SYS, _out, 3, HAL_MAX_DELAY);
    DELAY(BLOCK_DELAY);
    st25dv64k_unlock(); // unlock must be here because other threads cannot access eeprom while writing/waiting
}

void st25dv64k_present_pwd(uint8_t *pwd) {
    uint8_t _out[19] = { 0x09, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0x09, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (pwd) {
        memcpy(_out + 2, pwd, 8);
        memcpy(_out + 11, pwd, 8);
    }
    st25dv64k_lock();
    I2C_Transmit(&I2C_HANDLE_FOR(eeprom), ADDR_WRITE_SYS, _out, 19, HAL_MAX_DELAY);
    st25dv64k_unlock();
}
