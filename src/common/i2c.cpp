#include "i2c.hpp"
#include "stm32f4xx_hal.h"
#include "bsod.h"
#include "cmsis_os.h"
#include "bsod.h"
#include <type_traits>
#include <array>
#include "HAL/HAL.h"

#define MAX_RETRIES 20

namespace i2c {

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

static Result process_result(HAL_StatusTypeDef result) {
    switch (result) {
    case HAL_OK:
        return Result::ok;
    case HAL_ERROR:
        return Result::error;
    case HAL_BUSY:
        return Result::busy_after_retries;
    case HAL_TIMEOUT:
        return Result::timeout;
    default:
        fatal_error(ErrCode::ERR_ELECTRO_I2C_TX_UNDEFINED); // TODO change to ERR_ELECTRO_I2C_UNDEFINED
        break;
    }
    return Result::error; // will not get here, just prevent warning
}

// Helper class to disable the MOVE ISR (which can take up to 3ms) during I2C operations
class [[nodiscard]] MoveIsrDisabler {
    bool old_move_isr_state;

public:
    MoveIsrDisabler()
        : old_move_isr_state { MOVE_ISR_ENABLED() } {
        if (old_move_isr_state) {
            DISABLE_MOVE_INTERRUPT();
        }
    }

    ~MoveIsrDisabler() {
        if (old_move_isr_state) {
            ENABLE_MOVE_INTERRUPT();
        }
    }

    MoveIsrDisabler(const MoveIsrDisabler &) = delete;
    MoveIsrDisabler &operator=(const MoveIsrDisabler &) = delete;
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
        res = process_result(result);
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
        res = process_result(result);
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
        res = process_result(result);
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
        res = process_result(result);
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

    return process_result(result);
}

} // namespace i2c
