#include "i2c.hpp"
#include "stm32f4xx_hal.h"
#include "bsod.h"
#include "log.h"
#include "cmsis_os.h"
#include "bsod.h"
#include <type_traits>
#include <array>
#include "HAL/HAL.h"

#define MAX_RETRIES 20

LOG_COMPONENT_REF(EEPROM);

namespace i2c {

namespace statistics {

    struct Results {
        std::atomic<uint32_t> HAL_OK = 0;
        std::atomic<uint32_t> HAL_BUSY = 0;
        std::atomic<uint32_t> HAL_ERROR = 0;
        std::atomic<uint32_t> HAL_TIMEOUT = 0;
    };

#if HAS_I2CN(1)
    Results ch1;
#endif
#if HAS_I2CN(2)
    Results ch2;
#endif
#if HAS_I2CN(3)
    Results ch3;
#endif

    uint32_t get_hal_ok(uint8_t channel) {
        switch (channel) {
#if HAS_I2CN(1)
        case 1:
            return ch1.HAL_OK;
#endif
#if HAS_I2CN(2)
        case 2:
            return ch2.HAL_OK;
#endif
#if HAS_I2CN(3)
        case 3:
            return ch3.HAL_OK;
#endif
        }
        return 0;
    }

    uint32_t get_hal_busy(uint8_t channel) {
        switch (channel) {
#if HAS_I2CN(1)
        case 1:
            return ch1.HAL_BUSY;
#endif
#if HAS_I2CN(2)
        case 2:
            return ch2.HAL_BUSY;
#endif
#if HAS_I2CN(3)
        case 3:
            return ch3.HAL_BUSY;
#endif
        }
        return 0;
    }
    uint32_t get_hal_error(uint8_t channel) {
        switch (channel) {
#if HAS_I2CN(1)
        case 1:
            return ch1.HAL_ERROR;
#endif
#if HAS_I2CN(2)
        case 2:
            return ch2.HAL_ERROR;
#endif
#if HAS_I2CN(3)
        case 3:
            return ch3.HAL_ERROR;
#endif
        }
        return 0;
    }
    uint32_t get_hal_timeout(uint8_t channel) {
        switch (channel) {
#if HAS_I2CN(1)
        case 1:
            return ch1.HAL_TIMEOUT;
#endif
#if HAS_I2CN(2)
        case 2:
            return ch2.HAL_TIMEOUT;
#endif
#if HAS_I2CN(3)
        case 3:
            return ch3.HAL_TIMEOUT;
#endif
        }
        return 0;
    }
} // namespace statistics

namespace {
    std::array<osStaticMutexDef_t, 3> i2c_mutexes;
} // namespace

static int get_i2c_no(I2C_HandleTypeDef &hi2c) {
    if (&hi2c == &hi2c1) {
        return 1;
    } else if (&hi2c == &hi2c2) {
        return 2;
    } else if (&hi2c == &hi2c3) {
        return 3;
    } else {
        return -1;
    }
}

void ChannelMutex::static_init() {
    for (auto &m : i2c_mutexes) {
        if (!xSemaphoreCreateMutexStatic(&m)) {
            fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_UNDEFINED); // TODO change to ERR_I2C_MUTEX_CREATE_FAILED
        }
    }
}

osMutexId ChannelMutex::get_handle(I2C_HandleTypeDef &hi2c) {
    const auto i = get_i2c_no(hi2c) - 1;
    assert(i >= 0);
    return reinterpret_cast<osMutexId>(&i2c_mutexes[i]);
}

ChannelMutex::ChannelMutex(I2C_HandleTypeDef &hi2c)
    : mutex_handle(get_handle(hi2c)) {
    // lock
    if (mutex_handle) {
        osMutexWait(mutex_handle, osWaitForever);
    }
}

ChannelMutex::~ChannelMutex() {
    // unlock
    if (mutex_handle) {
        osMutexRelease(mutex_handle);
    }
}

static Result process_result_n(HAL_StatusTypeDef result, statistics::Results &result_counters) {
    switch (result) {
    case HAL_OK:
        ++result_counters.HAL_OK;
        log_debug(EEPROM, "%s: OK", __FUNCTION__);
        return Result::ok;
    case HAL_ERROR:
        ++result_counters.HAL_ERROR;
        log_error(EEPROM, "%s: ERROR", __FUNCTION__);
        return Result::error;
    case HAL_BUSY:
        ++result_counters.HAL_BUSY;
        log_error(EEPROM, "%s: BUSY", __FUNCTION__);
        return Result::busy_after_retries;
    case HAL_TIMEOUT:
        ++result_counters.HAL_TIMEOUT;
        log_error(EEPROM, "%s: TIMEOUT", __FUNCTION__);
        return Result::timeout;
    default:
        log_critical(EEPROM, "%s: UNDEFINED", __FUNCTION__);
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_UNDEFINED); // TODO change to ERR_ELECTRO_I2C_UNDEFINED
        break;
    }
    return Result::error; // will not get here, just prevent warning
}

static Result process_result(I2C_HandleTypeDef &hi2c, HAL_StatusTypeDef result) {
    int i2c_no = get_i2c_no(hi2c);

    switch (i2c_no) {
#if HAS_I2CN(1)
    case 1:
        return process_result_n(result, statistics::ch1);
#endif
#if HAS_I2CN(2)
    case 2:
        return process_result_n(result, statistics::ch2);
#endif
#if HAS_I2CN(3)
    case 3:
        return process_result_n(result, statistics::ch3);
#endif
    }

    fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_UNDEFINED); // TODO change to access to unused i2c
    return Result::error; // will not get here, just prevent warning
}

// Helper class to disable the MOVE ISR (which can take up to 3ms) during I2C operations
class [[nodiscard]] MoveIsrDisabler {
    bool old_move_isr_state;

public:
    MoveIsrDisabler() {
        old_move_isr_state = MOVE_ISR_ENABLED();
        if (old_move_isr_state) {
            DISABLE_MOVE_INTERRUPT();
        }
    }

    ~MoveIsrDisabler() {
        if (old_move_isr_state) {
            ENABLE_MOVE_INTERRUPT();
        }
    }
};

Result Transmit(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    int retries = MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    Result res = Result::error;
    while (retries--) {
        {
            ChannelMutex M(hi2c);
            MoveIsrDisabler moveIsrDisabler;
            result = HAL_I2C_Master_Transmit(&hi2c, DevAddress, pData, Size, Timeout);
        }
        res = process_result(hi2c, result);
        if (result != HAL_BUSY) {
            break;
        }
    }

    return res;
}

Result Receive(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {

    int retries = MAX_RETRIES;
    HAL_StatusTypeDef result = HAL_ERROR;
    Result res = Result::error;
    while (retries--) {
        {
            ChannelMutex M(hi2c);
            MoveIsrDisabler moveIsrDisabler;
            result = HAL_I2C_Master_Receive(&hi2c, DevAddress, pData, Size, Timeout);
        }
        res = process_result(hi2c, result);
        if (result != HAL_BUSY) {
            break;
        }
    }

    return res;
}

static Result Mem_Write(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    int retries = MAX_RETRIES; // prevent int underflow
    HAL_StatusTypeDef result = HAL_ERROR;
    Result res = Result::error;
    while (retries--) {
        {
            ChannelMutex M(hi2c);
            MoveIsrDisabler moveIsrDisabler;
            result = HAL_I2C_Mem_Write(&hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
        }
        res = process_result(hi2c, result);
        if (res == Result::ok) {
            break;
        }
    }

    return res;
}

Result Mem_Write_8bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return Mem_Write(hi2c, DevAddress, MemAddress, I2C_MEMADD_SIZE_8BIT, pData, Size, Timeout);
}

Result Mem_Write_16bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint16_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return Mem_Write(hi2c, DevAddress, MemAddress, I2C_MEMADD_SIZE_16BIT, pData, Size, Timeout);
}

[[nodiscard]] static Result Mem_Read(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    int retries = MAX_RETRIES; // prevent int underflow
    HAL_StatusTypeDef result = HAL_ERROR;
    Result res = Result::error;
    while (--retries) {
        {
            ChannelMutex M(hi2c);
            MoveIsrDisabler moveIsrDisabler;
            result = HAL_I2C_Mem_Read(&hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
        }
        res = process_result(hi2c, result);
        if (res != Result::ok) {
            break;
        }
    }

    return res;
}

Result Mem_Read_8bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint8_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return Mem_Read(hi2c, DevAddress, MemAddress, I2C_MEMADD_SIZE_8BIT, pData, Size, Timeout);
}

Result Mem_Read_16bit_Addr(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint16_t MemAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return Mem_Read(hi2c, DevAddress, MemAddress, I2C_MEMADD_SIZE_16BIT, pData, Size, Timeout);
}

Result IsDeviceReady(I2C_HandleTypeDef &hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout) {
    HAL_StatusTypeDef result;
    {
        ChannelMutex M(hi2c);
        result = HAL_I2C_IsDeviceReady(&hi2c, DevAddress, Trials, Timeout);
    }

    return process_result(hi2c, result);
}

} // namespace i2c
