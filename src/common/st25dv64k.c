// st25dv64k.c

#include "st25dv64k.h"
#include <string.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "bsod.h"

volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_OK = 0;
volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_ERROR = 0;
volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_BUSY = 0;
volatile uint32_t I2C_TRANSMIT_RESULTS_HAL_TIMEOUT = 0;
volatile uint32_t I2C_TRANSMIT_RESULTS_UNDEF = 0;

static void Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {

    int retries = 10;
    HAL_StatusTypeDef result;
    while (retries) {
        result = HAL_I2C_Master_Transmit(hi2c, DevAddress, pData, Size, Timeout);
        if (result != HAL_BUSY)
            break;
    }

    switch (result) {
    case HAL_OK:
        ++I2C_TRANSMIT_RESULTS_HAL_OK;
        break;
    case HAL_ERROR:
        ++I2C_TRANSMIT_RESULTS_HAL_ERROR;
        general_error("EEPROM Transmit", "I2C error");
        break;
    case HAL_BUSY:
        ++I2C_TRANSMIT_RESULTS_HAL_BUSY;
        general_error("EEPROM Transmit", "I2C busy");
        break;
    case HAL_TIMEOUT:
        ++I2C_TRANSMIT_RESULTS_HAL_TIMEOUT;
        general_error("EEPROM Transmit", "I2C timeout");
        break;
    default:
        ++I2C_TRANSMIT_RESULTS_UNDEF;
        general_error("EEPROM Transmit", "I2C undefined error");
        break;
    }
}

volatile uint32_t I2C_RECEIVE_RESULTS_HAL_OK = 0;
volatile uint32_t I2C_RECEIVE_RESULTS_HAL_ERROR = 0;
volatile uint32_t I2C_RECEIVE_RESULTS_HAL_BUSY = 0;
volatile uint32_t I2C_RECEIVE_RESULTS_HAL_TIMEOUT = 0;
volatile uint32_t I2C_RECEIVE_RESULTS_UNDEF = 0;

static void Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {

    int retries = 10;
    HAL_StatusTypeDef result;
    while (retries) {
        result = HAL_I2C_Master_Receive(hi2c, DevAddress, pData, Size, Timeout);
        if (result != HAL_BUSY)
            break;
    }

    switch (result) {
    case HAL_OK:
        ++I2C_RECEIVE_RESULTS_HAL_OK;
        break;
    case HAL_ERROR:
        ++I2C_RECEIVE_RESULTS_HAL_ERROR;
        general_error("EEPROM Receive", "I2C error");
        break;
    case HAL_BUSY:
        ++I2C_RECEIVE_RESULTS_HAL_BUSY;
        general_error("EEPROM Receive", "I2C busy");
        break;
    case HAL_TIMEOUT:
        ++I2C_RECEIVE_RESULTS_HAL_TIMEOUT;
        general_error("EEPROM Receive", "I2C timeout");
        break;
    default:
        ++I2C_RECEIVE_RESULTS_UNDEF;
        general_error("EEPROM Receive", "I2C undefined error");
        break;
    }
}

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
    uint8_t _out[2] = { address >> 8, address & 0xff };
    uint8_t data;
    st25dv64k_lock();
    Transmit(&EEPROM_I2C, ADDR_WRITE, _out, 2, HAL_MAX_DELAY);
    Receive(&EEPROM_I2C, ADDR_READ, &data, 1, HAL_MAX_DELAY);
    st25dv64k_unlock();
    return data;
}

void st25dv64k_user_write(uint16_t address, uint8_t data) {
    uint8_t _out[3] = { address >> 8, address & 0xff, data };
    st25dv64k_lock();
    Transmit(&EEPROM_I2C, ADDR_WRITE, _out, 3, HAL_MAX_DELAY);
    DELAY(BLOCK_DELAY);
    st25dv64k_unlock(); // unlock must be here because other threads cannot access eeprom while writing/waiting
}

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size) {
    uint8_t _out[2] = { address >> 8, address & 0xff };
    st25dv64k_lock();
    Transmit(&EEPROM_I2C, ADDR_WRITE, _out, 2, HAL_MAX_DELAY);
    Receive(&EEPROM_I2C, ADDR_READ, pdata, size, HAL_MAX_DELAY);
    st25dv64k_unlock();
}

void st25dv64k_user_write_bytes(uint16_t address, void const *pdata, uint16_t size) {
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
        Transmit(&EEPROM_I2C, ADDR_WRITE, _out, 2 + block_size, HAL_MAX_DELAY);
        DELAY(BLOCK_DELAY);
        size -= block_size;
        address += block_size;
        p += block_size;
    }
    st25dv64k_unlock(); // unlock must be here because other threads cannot access eeprom while writing/waiting
}

uint8_t st25dv64k_rd_cfg(uint16_t address) {
    uint8_t _out[2] = { address >> 8, address & 0xff };
    uint8_t data;
    st25dv64k_lock();
    Transmit(&EEPROM_I2C, ADDR_WRITE_SYS, _out, 2, HAL_MAX_DELAY);
    Receive(&EEPROM_I2C, ADDR_READ_SYS, &data, 1, HAL_MAX_DELAY);
    st25dv64k_unlock();
    return data;
}

void st25dv64k_wr_cfg(uint16_t address, uint8_t data) {
    uint8_t _out[3] = { address >> 8, address & 0xff, data };
    st25dv64k_lock();
    Transmit(&EEPROM_I2C, ADDR_WRITE_SYS, _out, 3, HAL_MAX_DELAY);
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
    Transmit(&EEPROM_I2C, ADDR_WRITE_SYS, _out, 19, HAL_MAX_DELAY);
    st25dv64k_unlock();
}
