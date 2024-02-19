/******************************************************************************
@file
@brief LIS2DH12 Arduino and Teensy Driver

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

#pragma once

#include "stdint.h"
#include "hwio_pindef.h"
#include <device/hal.h>

/**
 * Return values
 */
typedef enum {
    IMU_SUCCESS,
    IMU_HW_ERROR,
    IMU_HW_BUSY,
    IMU_TIMEOUT,
    IMU_NOT_SUPPORTED,
    IMU_GENERIC_ERROR,
    IMU_OUT_OF_BOUNDS,
    IMU_ALL_ONES_WARNING,
    //...
} status_t;

/**
 * @brief core operational class of the driver
 *
 * It contains only read and write operations towards the IMU.
 * To use the higher level functions, use the class LIS3DH which inherits
 * this class.
 */
class LIS2DHCore {
private:
    const buddy::hw::OutputPin &chip_select_pin;

public:
    explicit LIS2DHCore(const buddy::hw::OutputPin &chip_sel_pin);

protected:
    status_t beginCore(void);
    status_t readRegisterRegion(uint8_t *outputPointer, uint8_t offset, uint8_t length);

    // readRegister reads one 8-bit register
    status_t readRegister(uint8_t *, uint8_t);

    // Reads two 8-bit regs, LSByte then MSByte order, and concatenates them.
    //   Acts as a 16-bit read operation
    status_t readRegisterInt16(int16_t *, uint8_t offset);

    // Writes an 8-bit byte;
    status_t writeRegister(uint8_t, uint8_t);

private:
    // Communication stuff
    friend class Fifo;
};

// This struct holds the settings the driver uses to do calculations
struct SensorSettings {
public:
    // ADC and Temperature settings
    uint8_t adcEnabled;
    uint8_t tempEnabled;

    // Accelerometer settings
    uint16_t accelSampleRate; // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
    uint8_t accelRange; // Max G force readable.  Can be: 2, 4, 8, 16

    uint8_t xAccelEnabled;
    uint8_t yAccelEnabled;
    uint8_t zAccelEnabled;

    // Fifo settings
    uint8_t fifoEnabled;
    uint8_t fifoMode; // can be 0x0,0x1,0x2,0x3
    uint8_t fifoThreshold;
};

/**
 * @brief Highest level class of the driver.
 *
 * inherits the core and makes use of the beginCore()
 * method through it's own begin() method.  It also contains the
 * settings struct to hold user settings.
 */
class LIS2DH : public LIS2DHCore {
public:
    // IMU settings
    SensorSettings m_settings;

    // Error checking
    uint16_t m_allOnesCounter;
    uint16_t m_nonSuccessCounter;

    // Constructor generates default SensorSettings.
    //(over-ride after construction if desired)
    explicit LIS2DH(const buddy::hw::OutputPin &chip_sel_pin);
    //~LIS3DH() = default;

    // Call to apply SensorSettings
    status_t begin(void);
    void end(void);
    void applySettings(void);

    // Returns the raw bits from the sensor cast as 16-bit signed integers
    int16_t readRawAccelX(void);
    int16_t readRawAccelY(void);
    int16_t readRawAccelZ(void);

    // Returns the values as floats.  Inside, this calls readRaw___();
    float readFloatAccelX(void);
    float readFloatAccelY(void);
    float readFloatAccelZ(void);

    // FIFO stuff
    void fifoBegin(void);
    status_t fifoClear();
    status_t fifoGetStatus(uint8_t *);

    float calcAccel(int16_t);

    bool isSetupDone() const {
        return m_isInicialized;
    }

private:
    static constexpr bool m_high_resolution = false;
    bool m_isInicialized = false;
};

class Fifo {
public:
    struct Acceleration {
        float val[3];
    };
    Fifo(LIS2DH &accelerometer)
        : m_accelerometer(accelerometer)
        , m_num_records(0)
        , m_record_index_to_get(0)
        , m_state(State::draining)
        , m_succeded_samples { 0 }
        , m_sampling_start_time(0)
        , m_samples_taken(0) {}
    int get(Acceleration &acceleration);
    float get_sampling_rate();

private:
    struct Record {
        int16_t raw_x;
        int16_t raw_y;
        int16_t raw_z;
    };
    enum class State : uint8_t {
        draining,
        request_sent,
    };
    Acceleration to_acceleration(Record record);
    LIS2DH &m_accelerometer;
    Record m_records[32];
    static_assert(192 == sizeof(m_records), "No padding allowed.");
    int8_t m_num_records;
    int8_t m_record_index_to_get;
    State m_state;
    uint32_t m_succeded_samples;

    uint32_t m_sampling_start_time;
    uint32_t m_samples_taken;
};
