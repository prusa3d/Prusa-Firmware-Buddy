#include "i2c.h"
#include "bsod.h"
#include "log.h"
#include "cmsis_os.h"
#include "bsod_gui.hpp"

#define EEPROM_MAX_RETRIES 20

LOG_COMPONENT_REF(EEPROM);

volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_OK = 0;
volatile std::atomic<uint32_t> I2C_TRANSMIT_RESULTS_HAL_BUSY = 0;

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

extern "C" HAL_StatusTypeDef I2C_Transmit_ext(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    int retries = EEPROM_MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    while (--retries) {
        I2C_lock();
        result = HAL_I2C_Master_Transmit(hi2c, DevAddress, pData, Size, Timeout);
        I2C_unlock();
        if (result != HAL_BUSY)
            break;
        ++I2C_TRANSMIT_RESULTS_HAL_BUSY;
        log_error(EEPROM, "%s: was BUSY at %d. try of %d, total retries %d", __FUNCTION__, EEPROM_MAX_RETRIES - retries, retries, I2C_TRANSMIT_RESULTS_HAL_BUSY);
    }

    return result;
}

extern "C" void I2C_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    auto result = I2C_Transmit_ext(hi2c, DevAddress, pData, Size, Timeout);

    if (result == HAL_OK) {
        // print entire status only at log severity debug
        log_debug(EEPROM, "%s: OK %u, BUSY %u", __FUNCTION__,
            I2C_TRANSMIT_RESULTS_HAL_OK.load(), I2C_TRANSMIT_RESULTS_HAL_BUSY.load());
    } else {
        // some kind of error print entire status
        log_info(EEPROM, "%s: OK %u, BUSY %u", __FUNCTION__,
            I2C_TRANSMIT_RESULTS_HAL_OK.load(), I2C_TRANSMIT_RESULTS_HAL_BUSY.load());
    }

    switch (result) {
    case HAL_OK:
        ++I2C_TRANSMIT_RESULTS_HAL_OK;
        log_debug(EEPROM, "%s: OK", __FUNCTION__);
        break;
    case HAL_ERROR:
        log_error(EEPROM, "%s: ERROR", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_ERROR);
        break;
    case HAL_BUSY:
        log_error(EEPROM, "%s: BUSY", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_BUSY);
        break;
    case HAL_TIMEOUT:
        log_error(EEPROM, "%s: TIMEOUT", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_TIMEOUT);
        break;
    default:
        log_critical(EEPROM, "%s: UNDEFINED", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_UNDEFINED);
        break;
    }
}

volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_OK = 0;
volatile std::atomic<uint32_t> I2C_RECEIVE_RESULTS_HAL_BUSY = 0;

extern "C" void I2C_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {

    int retries = EEPROM_MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    while (--retries) {
        I2C_lock();
        result = HAL_I2C_Master_Receive(hi2c, DevAddress, pData, Size, Timeout);
        I2C_unlock();
        if (result != HAL_BUSY)
            break;
        ++I2C_RECEIVE_RESULTS_HAL_BUSY;
        log_error(EEPROM, "%s: was BUSY at %d. try of %d, total retries %d", __FUNCTION__, EEPROM_MAX_RETRIES - retries, retries, I2C_TRANSMIT_RESULTS_HAL_BUSY);
    }

    if (result == HAL_OK) {
        // print entire status only at log severity debug
        log_debug(EEPROM, "%s: OK %d, BUSY %d", __FUNCTION__,
            I2C_RECEIVE_RESULTS_HAL_OK.load(), I2C_RECEIVE_RESULTS_HAL_BUSY.load());
    } else {
        // some kind of error print entire status
        log_info(EEPROM, "%s: OK %d, BUSY %d", __FUNCTION__,
            I2C_RECEIVE_RESULTS_HAL_OK.load(), I2C_RECEIVE_RESULTS_HAL_BUSY.load());
    }

    switch (result) {
    case HAL_OK:
        ++I2C_RECEIVE_RESULTS_HAL_OK;
        log_debug(EEPROM, "%s: OK", __FUNCTION__);
        break;
    case HAL_ERROR:
        log_error(EEPROM, "%s: ERROR", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_RX_ERROR);
        break;
    case HAL_BUSY:
        log_error(EEPROM, "%s: BUSY", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_RX_BUSY);
        break;
    case HAL_TIMEOUT:
        log_error(EEPROM, "%s: TIMEOUT", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_RX_TIMEOUT);
        break;
    default:
        log_critical(EEPROM, "%s: UNDEFINED", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_RX_UNDEFINED);
        break;
    }
}
