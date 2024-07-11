/******************************************************************************
@file
@brief LIS2DH Arduino and Teensy Driver

Marshall Taylor @ SparkFun Electronics
Nov 16, 2016
https://github.com/sparkfun/LIS3DH_Breakout
https://github.com/sparkfun/SparkFun_LIS3DH_Arduino_Library

Resources:
Uses Wire.h for i2c operation
Uses SPI.h for SPI operation
Either can be omitted if not used

Development environment specifics:
Arduino IDE 1.6.4
Teensy loader 1.23

This code is released under the [MIT License](http://opensource.org/licenses/MIT).

Please review the LICENSE.md file included with this example. If you have any questions
or concerns with licensing, please contact techsupport@sparkfun.com.

Distributed as-is; no warranty is given.
******************************************************************************/
// Use VERBOSE_SERIAL to add debug serial to an existing Serial object.
// Note:  Use of VERBOSE_SERIAL adds delays surround RW ops, and should not be used
// for functional testing.
// #define VERBOSE_SERIAL

// See header file for additional topology notes.
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "SparkFunLIS2DH.h"

#include "Wire.h"
#include "SPI.h"
#include "hwio_pindef.h"
#include "timing_precise.hpp"
#include <common/scope_guard.hpp>
#include <device/peripherals.h>
#include <bit>
#include "Marlin/src/core/serial.h"

using namespace buddy::hw;

// Device Registers
#define LIS2DH_STATUS_REG_AUX  0x07
#define LIS2DH_OUT_TEMP_L      0x0C
#define LIS2DH_OUT_TEMP_H      0x0D
#define LIS2DH_INT_COUNTER_REG 0x0E // undocumented
#define LIS2DH_WHO_AM_I        0x0F

#define LIS2DH_CTRL_REG0     0x1E
#define LIS2DH_TEMP_CFG_REG  0x1F
#define LIS2DH_CTRL_REG1     0x20
#define LIS2DH_CTRL_REG2     0x21
#define LIS2DH_CTRL_REG3     0x22
#define LIS2DH_CTRL_REG4     0x23
#define LIS2DH_CTRL_REG5     0x24
#define LIS2DH_CTRL_REG6     0x25
#define LIS2DH_REFERENCE     0x26
#define LIS2DH_STATUS_REG2   0x27
#define LIS2DH_OUT_X_L       0x28
#define LIS2DH_OUT_X_H       0x29
#define LIS2DH_OUT_Y_L       0x2A
#define LIS2DH_OUT_Y_H       0x2B
#define LIS2DH_OUT_Z_L       0x2C
#define LIS2DH_OUT_Z_H       0x2D
#define LIS2DH_FIFO_CTRL_REG 0x2E
#define LIS2DH_FIFO_SRC_REG  0x2F
#define LIS2DH_INT1_CFG      0x30
#define LIS2DH_INT1_SRC      0x31
#define LIS2DH_INT1_THS      0x32
#define LIS2DH_INT1_DURATION 0x33
#define LIS2DH_INT2_CFG      0x34
#define LIS2DH_INT2_SRC      0x35
#define LIS2DH_INT2_THS      0x36
#define LIS2DH_INT2_DURATION 0x37

#define LIS2DH_CLICK_CFG    0x38
#define LIS2DH_CLICK_SRC    0x39
#define LIS2DH_CLICK_THS    0x3A
#define LIS2DH_TIME_LIMIT   0x3B
#define LIS2DH_TIME_LATENCY 0x3C
#define LIS2DH_TIME_WINDOW  0x3D
#define LIS2DH_ACT_THS      0x3E
#define LIS2DH_ACT_DUR      0x3F

static constexpr uint32_t HAL_SPI_TIMEOUT = 5;

[[nodiscard]] static status_t spi_transmit(uint8_t *data, size_t size) {
    if (HAL_SPI_Transmit(&SPI_HANDLE_FOR(accelerometer), data, size, HAL_SPI_TIMEOUT) == HAL_OK) {
        return IMU_SUCCESS;
    } else {
        return IMU_GENERIC_ERROR;
    }
}

[[nodiscard]] static status_t spi_receive(uint8_t *data, size_t size) {
    if (HAL_SPI_Receive(&SPI_HANDLE_FOR(accelerometer), data, size, HAL_SPI_TIMEOUT) == HAL_OK) {
        return IMU_SUCCESS;
    } else {
        return IMU_GENERIC_ERROR;
    }
}

#define TRY_SPI_TRANSMIT(data, size)                                               \
    if (const status_t status = spi_transmit(data, size); status != IMU_SUCCESS) { \
        return status;                                                             \
    }

#define TRY_SPI_RECEIVE(data, size)                                               \
    if (const status_t status = spi_receive(data, size); status != IMU_SUCCESS) { \
        return status;                                                            \
    }

class ChipSelect {
private:
    const buddy::hw::OutputPin &pin;

public:
    ChipSelect(const buddy::hw::OutputPin &pin)
        : pin { pin } {
        // Ensure minimum deselect time from previous transfer.
        // The chip might be deselected in ISR so the minimum delay is not
        // in ISR but here.
        delay_ns_precise<50>();
        // take the chip select low to select the device:
        pin.write(Pin::State::low);
        delay_ns_precise<5>();
    }
    ~ChipSelect() {
        // take the chip select high to de-select:
        delay_ns_precise<20>();
        pin.write(Pin::State::high);
    }
};

//****************************************************************************//
//
//  LIS3DHCore functions.
//
//****************************************************************************//
LIS2DHCore::LIS2DHCore(const buddy::hw::OutputPin &chip_sel_pin)
    : chip_select_pin { chip_sel_pin } {
}

status_t LIS2DHCore::beginCore(void) {
    // Check the ID register to determine if we have an accelerometer
    uint8_t readCheck;
    readRegister(&readCheck, LIS2DH_WHO_AM_I);
    if (readCheck != 0x33) {
        return IMU_HW_ERROR;
    }

    // Power down mode
    writeRegister(LIS2DH_CTRL_REG1, 0);

    // Reset FIFO mode to bypass in order to reset FIFO
    writeRegister(LIS2DH_FIFO_CTRL_REG, 0);

    // Soft-reset device to ensure fresh state. Reboot memory content.
    writeRegister(LIS2DH_CTRL_REG5, 0b10000000);

    /*
     * After connecting the accelerometer, the first memory reboot doesn't
     * clear the boot bit in the CTRL_REG5 and the accelerometer stays in a
     * state where it returns only zeros for all measurements.
     *
     * The only working solution, for now, is to give the accelerometer a bit
     * of time to complete the memory reboot and then reset the boot bit in
     * CTRL_REG5 back to zero manually.
     *
     * Unsuccesful attempts:
     * - just wait longer
     * - reset again after a short wait
     * - read/write stuff from/to CTRL registers
     */
    osDelay(25);
    writeRegister(LIS2DH_CTRL_REG5, 0);

    return IMU_SUCCESS;
}

/**
 * @brief  Read a chunk of memory
 *
 * Does not know if the target memory space is an array or not, or
 * if there is the array is big enough.  if the variable passed is only
 * two bytes long and 3 bytes are requested, this will over-write some
 * other memory!
 *
 * @param outputPointer Pass &variable (base address of) to save read data to
 * @param offset register to read
 * @param length number of bytes to read
 * @return status_t
 */
status_t LIS2DHCore::readRegisterRegion(uint8_t *outputPointer, uint8_t offset, uint8_t length) {
    status_t returnError = IMU_SUCCESS;

    // define pointer that will point to the external space
    uint8_t i = 0;
    uint8_t c = 0;
    uint8_t tempFFCounter = 0;

    ChipSelect chip_select { chip_select_pin };
    // send the device the register you want to read:
    offset = offset | 0x80 | 0x40; // Ored with "read request" bit and "auto increment" bit
    TRY_SPI_TRANSMIT(&offset, 1);
    while (i < length) // slave may send less than requested
    {
        TRY_SPI_RECEIVE(&c, 1);
        if (c == 0xFF) {
            // May have problem
            tempFFCounter++;
        }
        *outputPointer = c;
        outputPointer++;
        i++;
    }
    if (i > 0 && tempFFCounter == i) {
        // Ok, we've recieved all ones, report
        returnError = IMU_ALL_ONES_WARNING;
    }

    return returnError;
}

//****************************************************************************//
//
//  ReadRegister
//
//  Parameters:
//    *outputPointer -- Pass &variable (address of) to save read data to
//    offset -- register to read
//
//****************************************************************************//
status_t LIS2DHCore::readRegister(uint8_t *outputPointer, uint8_t offset) {
    // Return value
    uint8_t result;
    status_t returnError = IMU_SUCCESS;

    ChipSelect chip_select { chip_select_pin };
    // send the device the register you want to read:
    offset = offset | 0x80; // Ored with "read request" bit
    TRY_SPI_TRANSMIT(&offset, 1);
    TRY_SPI_RECEIVE(&result, 1);

    if (result == 0xFF) {
        // we've recieved all ones, report
        returnError = IMU_ALL_ONES_WARNING;
    }

    *outputPointer = result;
    return returnError;
}

//****************************************************************************//
//
//  readRegisterInt16
//
//  Parameters:
//    *outputPointer -- Pass &variable (base address of) to save read data to
//    offset -- register to read
//
//****************************************************************************//
status_t LIS2DHCore::readRegisterInt16(int16_t *outputPointer, uint8_t offset) {
    {
        uint8_t myBuffer[2];
        status_t returnError = readRegisterRegion(myBuffer, offset, 2); // Does memory transfer
        int16_t output = (int16_t)myBuffer[0] | int16_t(myBuffer[1] << 8);
        *outputPointer = output;
        return returnError;
    }
}

//****************************************************************************//
//
//  writeRegister
//
//  Parameters:
//    offset -- register to write
//    dataToWrite -- 8 bit data to write to register
//
//****************************************************************************//
status_t LIS2DHCore::writeRegister(uint8_t offset, uint8_t dataToWrite) {
    ChipSelect chip_select { chip_select_pin };
    // send the device the register you want to read:
    TRY_SPI_TRANSMIT(&offset, 1);
    TRY_SPI_TRANSMIT(&dataToWrite, 1);
    return IMU_SUCCESS;
}

//****************************************************************************//
//
//  Main user class -- wrapper for the core class + maths
//
//****************************************************************************//
LIS2DH::LIS2DH(const buddy::hw::OutputPin &chip_sel_pin)
    : LIS2DHCore { chip_sel_pin } {
    // Construct with these default settings
    // ADC stuff
    m_settings.adcEnabled = 0;

    // Temperature settings
    m_settings.tempEnabled = 0;

    // Accelerometer settings
    m_settings.accelSampleRate = 1344; // Hz.  Can be: 0,1,10,25,50,100,200,400,1344,1600,5000 Hz
    m_settings.accelRange = 2; // Max G force readable.  Can be: 2, 4, 8, 16

    m_settings.xAccelEnabled = 1;
    m_settings.yAccelEnabled = 1;
    m_settings.zAccelEnabled = 1;

    // FIFO control settings
    m_settings.fifoEnabled = 1; // enabled
    m_settings.fifoThreshold = 20; // Can be 0 to 32
    m_settings.fifoMode = 2; // Stream mode.

    m_allOnesCounter = 0;
    m_nonSuccessCounter = 0;
    m_isInicialized = false;
}

//****************************************************************************//
//
//  Begin
//
//  This starts the lower level begin, then applies settings
//
//****************************************************************************//
status_t LIS2DH::begin(void) {
    // Begin the inherited core.  This gets the physical wires connected
    status_t returnError = beginCore();
    if (IMU_SUCCESS != returnError) {
        return returnError;
    }

    applySettings();
    m_isInicialized = true;
    return returnError;
}

void LIS2DH::end(void) {
    std::ignore = beginCore();
    m_isInicialized = false;
}

//****************************************************************************//
//
//  Configuration section
//
//  This uses the stored SensorSettings to start the IMU
//  Use statements such as "myIMU.settings.commInterface = SPI_MODE;" or
//  "myIMU.settings.accelEnabled = 1;" to configure before calling .begin();
//
//****************************************************************************//
void LIS2DH::applySettings(void) {
    uint8_t dataToWrite = 0; // Temporary variable

    // Build TEMP_CFG_REG
    dataToWrite = 0; // Start Fresh!
    dataToWrite = ((m_settings.tempEnabled & 0x01) << 6) | ((m_settings.adcEnabled & 0x01) << 7);
// Now, write the patched together data
#ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_TEMP_CFG_REG: 0x");
    Serial.println(dataToWrite, HEX);
#endif
    writeRegister(LIS2DH_TEMP_CFG_REG, dataToWrite);

    // Build CTRL_REG1
    dataToWrite = 0; // Start Fresh!
    //  Convert ODR
    switch (m_settings.accelSampleRate) {
    case 1:
        dataToWrite |= (0x01 << 4);
        break;
    case 10:
        dataToWrite |= (0x02 << 4);
        break;
    case 25:
        dataToWrite |= (0x03 << 4);
        break;
    case 50:
        dataToWrite |= (0x04 << 4);
        break;
    case 100:
        dataToWrite |= (0x05 << 4);
        break;
    case 200:
        dataToWrite |= (0x06 << 4);
        break;
    default:
    case 400:
        dataToWrite |= (0x07 << 4);
        break;
    case 1600:
        dataToWrite |= (0x08 << 4);
        break;
    case 1344:
    case 5000:
        dataToWrite |= (0x09 << 4);
        break;
    }

    dataToWrite |= (m_settings.zAccelEnabled & 0x01) << 2;
    dataToWrite |= (m_settings.yAccelEnabled & 0x01) << 1;
    dataToWrite |= (m_settings.xAccelEnabled & 0x01);
// Now, write the patched together data
#ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_CTRL_REG1: 0x");
    Serial.println(dataToWrite, HEX);
#endif
    writeRegister(LIS2DH_CTRL_REG1, dataToWrite);

    // Build CTRL_REG4
    dataToWrite = 0; // Start Fresh!
    //  Convert scaling
    switch (m_settings.accelRange) {
    case 2:
        dataToWrite |= (0x00 << 4);
        break;
    case 4:
        dataToWrite |= (0x01 << 4);
        break;
    case 8:
        dataToWrite |= (0x02 << 4);
        break;
    default:
    case 16:
        dataToWrite |= (0x03 << 4);
        break;
    }
    dataToWrite |= 0x80; // set block update
    if (m_high_resolution) {
        dataToWrite |= 0x08; // set high resolution
    }
#ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_CTRL_REG4: 0x");
    Serial.println(dataToWrite, HEX);
#endif
    // Now, write the patched together data
    writeRegister(LIS2DH_CTRL_REG4, dataToWrite);

    // Just in case reset filtering by reading reference register
    uint8_t dummy;
    readRegister(&dummy, LIS2DH_REFERENCE);
}
//****************************************************************************//
//
//  Accelerometer section
//
//****************************************************************************//
int16_t LIS2DH::readRawAccelX(void) {
    int16_t output;
    status_t errorLevel = readRegisterInt16(&output, LIS2DH_OUT_X_L);
    if (errorLevel != IMU_SUCCESS) {
        if (errorLevel == IMU_ALL_ONES_WARNING) {
            m_allOnesCounter++;
        } else {
            m_nonSuccessCounter++;
        }
    }
    return output;
}
float LIS2DH::readFloatAccelX(void) {
    float output = calcAccel(readRawAccelX());
    return output;
}

int16_t LIS2DH::readRawAccelY(void) {
    int16_t output;
    status_t errorLevel = readRegisterInt16(&output, LIS2DH_OUT_Y_L);
    if (errorLevel != IMU_SUCCESS) {
        if (errorLevel == IMU_ALL_ONES_WARNING) {
            m_allOnesCounter++;
        } else {
            m_nonSuccessCounter++;
        }
    }
    return output;
}

float LIS2DH::readFloatAccelY(void) {
    float output = calcAccel(readRawAccelY());
    return output;
}

int16_t LIS2DH::readRawAccelZ(void) {
    int16_t output;
    status_t errorLevel = readRegisterInt16(&output, LIS2DH_OUT_Z_L);
    if (errorLevel != IMU_SUCCESS) {
        if (errorLevel == IMU_ALL_ONES_WARNING) {
            m_allOnesCounter++;
        } else {
            m_nonSuccessCounter++;
        }
    }
    return output;
}

float LIS2DH::readFloatAccelZ(void) {
    float output = calcAccel(readRawAccelZ());
    return output;
}

/**
 * @brief Calculate acceleration from raw value
 *
 * @param raw acceleration from sensor
 * @return floating point acceleration in m/s^2
 */
float LIS2DH::calcAccel(int16_t input) {
    constexpr float standard_gravity = 9.80665f;
    constexpr int16_t max_value = 0b0111'1111'1111'1111;
    constexpr float factor2g = 2.f * standard_gravity / max_value;
    constexpr float factor4g = 4.f * standard_gravity / max_value;
    constexpr float factor8g = 8.f * standard_gravity / max_value;
    constexpr float factor16g = 16.f * standard_gravity / max_value;
    float output;
    switch (m_settings.accelRange) {
    case 2:
        output = input * factor2g;
        break;
    case 4:
        output = input * factor4g;
        break;
    case 8:
        output = input * factor8g;
        break;
    case 16:
        output = input * factor16g;
        break;
    default:
        output = 0;
        break;
    }
    return output;
}

//****************************************************************************//
//
//  FIFO section
//
//****************************************************************************//
void LIS2DH::fifoBegin(void) {
    uint8_t dataToWrite = 0; // Temporary variable

    // Build LIS3DH_FIFO_CTRL_REG
    readRegister(&dataToWrite, LIS2DH_FIFO_CTRL_REG); // Start with existing data

    dataToWrite &= 0x20; // clear all but bit 5
    dataToWrite |= (m_settings.fifoMode & 0x03) << 6; // apply mode Stream-to-FIFO
    dataToWrite |= (m_settings.fifoThreshold & 0x1F); // apply threshold
                                                      // Now, write the patched together data
#ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
    Serial.println(dataToWrite, HEX);
#endif
    writeRegister(LIS2DH_FIFO_CTRL_REG, dataToWrite);

    // Build CTRL_REG5
    readRegister(&dataToWrite, LIS2DH_CTRL_REG5); // Start with existing data
    dataToWrite &= 0xBF; // clear bit 6
    dataToWrite |= (m_settings.fifoEnabled & 0x01) << 6;
// Now, write the patched together data
#ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_CTRL_REG5: 0x");
    Serial.println(dataToWrite, HEX);
#endif
    writeRegister(LIS2DH_CTRL_REG5, dataToWrite);
}

status_t LIS2DH::fifoClear() {
    // Drain the fifo data and dump it
    for (;;) {
        uint8_t fifo_status;
        const status_t status = fifoGetStatus(&fifo_status);
        if (status == IMU_SUCCESS) {
            if ((fifo_status & 0x20) == 0) {
                readRawAccelX();
                readRawAccelY();
                readRawAccelZ();
            } else {
                return status;
            }
        } else {
            return status;
        }
    }
}

status_t LIS2DH::fifoGetStatus(uint8_t *tempReadByte) {
    // Return some data on the state of the fifo
    return readRegister(tempReadByte, LIS2DH_FIFO_SRC_REG);
}

/**
 * @brief Get acceleration in all axis
 *
 * To be called periodically. Returns fresh sample from buffer.
 * Whenever there are are no more fresh samples, DMA transfer is started
 * to get new fresh samples.
 *
 * @param[out] acceleration
 * @retval positive acceleration returned
 * @retval >1 we have more samples in the buffer
 * @retval 1 this is last fresh sample, fresh samples retrieval started
 * @retval 0 nothing returned
 */
int Fifo::get(Acceleration &acceleration) {
    int local_num_samples = 0;
    switch (m_state) {
    case State::request_sent:
        m_state = State::draining;
        // fall through
    case State::draining:
        local_num_samples = m_num_records - m_record_index_to_get;
        if (local_num_samples > 0) {
            acceleration = to_acceleration(m_records[m_record_index_to_get++]);
        }
        if (local_num_samples <= 1) {
            uint8_t fifo_status { 0b0100'0000 };
            {
                const status_t read_status = m_accelerometer.readRegister(&fifo_status, LIS2DH_FIFO_SRC_REG);
                if (!((read_status == IMU_SUCCESS) || (read_status == IMU_ALL_ONES_WARNING))) {
                    break;
                }
            }
            const bool overrun = fifo_status & 0b0100'0000;
            if (overrun) {
                SERIAL_ERROR_MSG("Overrun.");
                SERIAL_ECHO_START();
                SERIAL_ECHOLNPAIR_F("After successfully received samples:", m_succeded_samples);
                m_succeded_samples = 0;
            }

            const int remote_num_samples = (fifo_status & 0b0001'1111) + overrun;
            static_assert(std::endian::native == std::endian::little, "Byte swapping for 16-bit record.raw value not implemented.");

            const status_t returnError = m_accelerometer.readRegisterRegion(reinterpret_cast<uint8_t *>(m_records), LIS2DH_OUT_X_L, remote_num_samples * sizeof(Record));

            if (returnError != IMU_SUCCESS) {
                break;
            }
            assert(remote_num_samples >= 0);
            m_num_records = remote_num_samples;
            m_succeded_samples += remote_num_samples;
            m_record_index_to_get = 0;
        }
        break;
    }
    return local_num_samples;
}

Fifo::Acceleration Fifo::to_acceleration(Record record) {
    Acceleration retval;
#if PRINTER_IS_PRUSA_iX
    retval.val[0] = m_accelerometer.calcAccel(record.raw_y);
    retval.val[1] = m_accelerometer.calcAccel(record.raw_z);
    retval.val[2] = m_accelerometer.calcAccel(record.raw_x);
#else
    retval.val[0] = m_accelerometer.calcAccel(record.raw_x);
    retval.val[1] = m_accelerometer.calcAccel(record.raw_y);
    retval.val[2] = m_accelerometer.calcAccel(record.raw_z);
#endif
    return retval;
}
