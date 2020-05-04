// st25dv64k.c

#include "st25dv64k.h"
#include <string.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

//
#define ST25DV64K_RTOS

//system config address registr
#define REG_GPO         0x0000
#define REG_IT_TIME     0x0001
#define REG_EH_MODE     0x0002
#define REG_RF_MNGT     0x0003
#define REG_RFA1SS      0x0004
#define REG_ENDA1       0x0005
#define REG_RFA2SS      0x0006
#define REG_ENDA2       0x0007
#define REG_RFA3SS      0x0008
#define REG_ENDA3       0x0009
#define REG_RFA4SS      0x000A
#define REG_I2CSS       0x000B
#define REG_LOCK_CCFILE 0x000C
#define REG_MB_MODE     0x000D
#define REG_MB_WDG      0x000E
#define REG_LOCK_CFG    0x000F

// EEPROM I2C addresses
#define ADDR_WRITE     0xA6
#define ADDR_READ      0xA7
#define ADDR_WRITE_SYS 0xAE
#define ADDR_READ_SYS  0xAF

#define BLOCK_DELAY 5 // block delay [ms]
#define BLOCK_BYTES 4 // bytes per block

#define DELAY HAL_Delay

extern I2C_HandleTypeDef hi2c1;

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
    HAL_I2C_Master_Transmit(&hi2c1, ADDR_WRITE, _out, 2, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c1, ADDR_READ, &data, 1, HAL_MAX_DELAY);
    st25dv64k_unlock();
    return data;
}

void st25dv64k_user_write(uint16_t address, uint8_t data) {
    uint8_t _out[3] = { address >> 8, address & 0xff, data };
    st25dv64k_lock();
    HAL_I2C_Master_Transmit(&hi2c1, ADDR_WRITE, _out, 3, HAL_MAX_DELAY);
    DELAY(BLOCK_DELAY);
    st25dv64k_unlock(); // unlock must be here because other threads cannot access eeprom while writing/waiting
}

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint8_t size) {
    uint8_t _out[2] = { address >> 8, address & 0xff };
    st25dv64k_lock();
    HAL_I2C_Master_Transmit(&hi2c1, ADDR_WRITE, _out, 2, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c1, ADDR_READ, pdata, size, HAL_MAX_DELAY);
    st25dv64k_unlock();
}

void st25dv64k_user_write_bytes(uint16_t address, void *pdata, uint8_t size) {
    uint8_t *p = (uint8_t *)pdata;
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
        HAL_I2C_Master_Transmit(&hi2c1, ADDR_WRITE, _out, 2 + block_size, HAL_MAX_DELAY);
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
    HAL_I2C_Master_Transmit(&hi2c1, ADDR_WRITE_SYS, _out, 2, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive(&hi2c1, ADDR_READ_SYS, &data, 1, HAL_MAX_DELAY);
    st25dv64k_unlock();
    return data;
}

void st25dv64k_wr_cfg(uint16_t address, uint8_t data) {
    uint8_t _out[3] = { address >> 8, address & 0xff, data };
    st25dv64k_lock();
    HAL_I2C_Master_Transmit(&hi2c1, ADDR_WRITE_SYS, _out, 3, HAL_MAX_DELAY);
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
    HAL_I2C_Master_Transmit(&hi2c1, ADDR_WRITE_SYS, _out, 19, HAL_MAX_DELAY);
    st25dv64k_unlock();
}
