#include "i2c.hpp"
#include "bsod.h"
#include "log.h"
#include "cmsis_os.h"
#include "bsod_gui.hpp"
#include "inc/MarlinConfig.h"

#define EEPROM_MAX_RETRIES 20

LOG_COMPONENT_REF(EEPROM);

namespace i2c {

namespace statistics {
    std::atomic<uint32_t> transmit_HAL_OK = 0;
    std::atomic<uint32_t> transmit_HAL_BUSY = 0;
    std::atomic<uint32_t> transmit_HAL_ERROR = 0;
    std::atomic<uint32_t> transmit_HAL_TIMEOUT = 0;
    std::atomic<uint32_t> receive_HAL_OK = 0;
    std::atomic<uint32_t> receive_HAL_BUSY = 0;
    std::atomic<uint32_t> receive_HAL_ERROR = 0;
    std::atomic<uint32_t> receive_HAL_TIMEOUT = 0;

    uint32_t get_transmit_HAL_OK() { return transmit_HAL_OK; }
    uint32_t get_transmit_HAL_BUSY() { return transmit_HAL_BUSY; }
    uint32_t get_transmit_HAL_ERROR() { return transmit_HAL_ERROR; }
    uint32_t get_transmit_HAL_TIMEOUT() { return transmit_HAL_TIMEOUT; }
    uint32_t get_receive_HAL_OK() { return receive_HAL_OK; }
    uint32_t get_receive_HAL_BUSY() { return receive_HAL_BUSY; }
    uint32_t get_receive_HAL_ERROR() { return receive_HAL_ERROR; }
    uint32_t get_receive_HAL_TIMEOUT() { return receive_HAL_TIMEOUT; }
} // namespace statistics

using namespace statistics;

osMutexId i2c_mutex = 0; // mutex handle

static void I2C_lock(void) {
    if (i2c_mutex == 0) {
        osMutexDef(i2c_mutex);
        i2c_mutex = osMutexCreate(osMutex(i2c_mutex));
    }
    osMutexWait(i2c_mutex, osWaitForever);
}

static void I2C_unlock(void) {
    osMutexRelease(i2c_mutex);
}

HAL_StatusTypeDef Transmit_ext(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    int retries = EEPROM_MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    while (--retries) {
        I2C_lock();
        // Disable the move interrupt to ensure that I2C transmit will not be interrupted by the move interrupt that can take a couple of milliseconds.
        const bool enabled_move_isr = MOVE_ISR_ENABLED();
        if (enabled_move_isr)
            DISABLE_MOVE_INTERRUPT();

        result = HAL_I2C_Master_Transmit(hi2c, DevAddress, pData, Size, Timeout);
        if (enabled_move_isr)
            ENABLE_MOVE_INTERRUPT();

        I2C_unlock();
        if (result != HAL_BUSY)
            break;
        ++transmit_HAL_BUSY;
        log_error(EEPROM, "%s: was BUSY at %d. try of %d, total retries %d", __FUNCTION__, EEPROM_MAX_RETRIES - retries, retries, transmit_HAL_BUSY.load());
    }

    return result;
}

Result Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    auto result = Transmit_ext(hi2c, DevAddress, pData, Size, Timeout);

    switch (result) {
    case HAL_OK:
        ++transmit_HAL_OK;
        log_debug(EEPROM, "%s: OK", __FUNCTION__);
        return Result::ok;
    case HAL_ERROR:
        ++transmit_HAL_ERROR;
        log_error(EEPROM, "%s: ERROR", __FUNCTION__);
        return Result::error;
    case HAL_BUSY:
        ++transmit_HAL_BUSY;
        log_error(EEPROM, "%s: BUSY", __FUNCTION__);
        return Result::busy_after_retries;
    case HAL_TIMEOUT:
        ++transmit_HAL_TIMEOUT;
        log_error(EEPROM, "%s: TIMEOUT", __FUNCTION__);
        return Result::timeout;
    default:
        log_critical(EEPROM, "%s: UNDEFINED", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_UNDEFINED);
        break;
    }
    return Result::error; // will not get here, just prevent warning
}

Result Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {

    int retries = EEPROM_MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    while (--retries) {
        I2C_lock();
        // Disable the move interrupt to ensure that I2C receive will not be interrupted by the move interrupt that can take a couple of milliseconds.
        const bool enabled_move_isr = MOVE_ISR_ENABLED();
        if (enabled_move_isr)
            DISABLE_MOVE_INTERRUPT();

        result = HAL_I2C_Master_Receive(hi2c, DevAddress, pData, Size, Timeout);
        if (enabled_move_isr)
            ENABLE_MOVE_INTERRUPT();

        I2C_unlock();
        if (result != HAL_BUSY)
            break;
        ++receive_HAL_BUSY;
        log_error(EEPROM, "%s: was BUSY at %d. try of %d, total retries %d", __FUNCTION__, EEPROM_MAX_RETRIES - retries, retries, receive_HAL_BUSY.load());
    }

    switch (result) {
    case HAL_OK:
        ++receive_HAL_OK;
        log_debug(EEPROM, "%s: OK", __FUNCTION__);
        return Result::ok;
    case HAL_ERROR:
        ++receive_HAL_ERROR;
        log_error(EEPROM, "%s: ERROR", __FUNCTION__);
        return Result::error;
    case HAL_BUSY:
        ++receive_HAL_BUSY;
        log_error(EEPROM, "%s: BUSY", __FUNCTION__);
        return Result::busy_after_retries;
    case HAL_TIMEOUT:
        ++receive_HAL_TIMEOUT;
        log_error(EEPROM, "%s: TIMEOUT", __FUNCTION__);
        return Result::timeout;
    default:
        log_critical(EEPROM, "%s: UNDEFINED", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_RX_UNDEFINED);
    }
    return Result::error; // will not get here, just prevent warning
}
} // namespace i2c
