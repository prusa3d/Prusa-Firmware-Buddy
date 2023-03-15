/******************************************************************************
SparkFunLIS3DH.cpp
LIS3DH Arduino and Teensy Driver

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
//Use VERBOSE_SERIAL to add debug serial to an existing Serial object.
//Note:  Use of VERBOSE_SERIAL adds delays surround RW ops, and should not be used
//for functional testing.
//#define VERBOSE_SERIAL

//See SparkFunLIS3DH.h for additional topology notes.
#include "config_buddy_2209_02.h"
#ifdef HAS_ACCELEROMETR
    #include "SparkFunLIS3DH.h"
    #include "stdint.h"

    #include "Wire.h"
    #include "SPI.h"
    #include "hwio_pindef.h"
    #include "main.h"

using namespace buddy::hw;
//****************************************************************************//
//
//  LIS3DHCore functions.
//
//  Construction arguments:
//  ( uint8_t busType, uint8_t inputArg ),
//
//    where inputArg is address for I2C_MODE and chip select pin
//    number for SPI_MODE
//
//  For SPI, construct LIS3DHCore myIMU(SPI_MODE, 10);
//  For I2C, construct LIS3DHCore myIMU(I2C_MODE, 0x6B);
//
//  Default construction is I2C mode, address 0x6B.
//
//****************************************************************************//
LIS3DHCore::LIS3DHCore(uint8_t busType, uint8_t inputArg)
    : commInterface(I2C_MODE)
    , I2CAddress(0x19)
    , chipSelectPin(10) {
    commInterface = busType;
    if (commInterface == I2C_MODE) {
        I2CAddress = inputArg;
    }
    if (commInterface == SPI_MODE) {
        chipSelectPin = inputArg;
    }
}

status_t LIS3DHCore::beginCore(void) {
    status_t returnError = IMU_SUCCESS;

    switch (commInterface) {

    case I2C_MODE:
        Wire.begin();
        break;

    case SPI_MODE:
    #if defined(ARDUINO_ARCH_ESP32)
        // initalize the chip select pins:
        pinMode(chipSelectPin, OUTPUT);
        digitalWrite(chipSelectPin, HIGH);
        SPI.begin();
        SPI.setFrequency(1000000);
        // Data is read and written MSb first.
        SPI.setBitOrder(SPI_MSBFIRST);
        // Like the standard arduino/teensy comment below, mode0 seems wrong according to standards
        // but conforms to the timing diagrams when used for the ESP32
        SPI.setDataMode(SPI_MODE0);

    #elif defined(__MK20DX256__)
        // initalize the chip select pins:
        pinMode(chipSelectPin, OUTPUT);
        digitalWrite(chipSelectPin, HIGH);
        // start the SPI library:
        SPI.begin();
        // Maximum SPI frequency is 10MHz, could divide by 2 here:
        SPI.setClockDivider(SPI_CLOCK_DIV4);
        // Data is read and written MSb first.
        SPI.setBitOrder(MSBFIRST);
        // Data is captured on rising edge of clock (CPHA = 0)
        // Base value of the clock is HIGH (CPOL = 1)

        // MODE0 for Teensy 3.1 operation
        SPI.setDataMode(SPI_MODE0);
    #else
            // probably __AVR__
            // initalize the chip select pins:
            //pinMode(chipSelectPin, OUTPUT);
            //digitalWrite(chipSelectPin, HIGH);
            // start the SPI library:
            //SPI.begin();
            // Maximum SPI frequency is 10MHz, could divide by 2 here:
            //SPI.setClockDivider(SPI_CLOCK_DIV4);
            // Data is read and written MSb first.
            //SPI.setBitOrder(MSBFIRST);
            // Data is captured on rising edge of clock (CPHA = 0)
            // Base value of the clock is HIGH (CPOL = 1)

            // MODE3 for 328p operation
            //SPI.setDataMode(SPI_MODE3);

    #endif
        break;
    default:
        break;
    }

    //Spin for a few ms
    volatile uint8_t temp = 0;
    for (uint16_t i = 0; i < 10000; i++) {
        temp++;
    }

    //Check the ID register to determine if the operation was a success.
    uint8_t readCheck;
    readRegister(&readCheck, LIS3DH_WHO_AM_I);
    if (readCheck != 0x33) {
        returnError = IMU_HW_ERROR;
    }

    return returnError;
}

//****************************************************************************//
//
//  ReadRegisterRegion
//
//  Parameters:
//    *outputPointer -- Pass &variable (base address of) to save read data to
//    offset -- register to read
//    length -- number of bytes to read
//
//  Note:  Does not know if the target memory space is an array or not, or
//    if there is the array is big enough.  if the variable passed is only
//    two bytes long and 3 bytes are requested, this will over-write some
//    other memory!
//
//****************************************************************************//
status_t LIS3DHCore::readRegisterRegion(uint8_t *outputPointer, uint8_t offset, uint8_t length) {
    status_t returnError = IMU_SUCCESS;

    //define pointer that will point to the external space
    uint8_t i = 0;
    uint8_t c = 0;
    uint8_t tempFFCounter = 0;

    switch (commInterface) {

    case I2C_MODE:
        Wire.beginTransmission(I2CAddress);
        offset |= 0x80; //turn auto-increment bit on, bit 7 for I2C
        Wire.write(offset);
        if (Wire.endTransmission() != 0) {
            returnError = IMU_HW_ERROR;
        } else //OK, all worked, keep going
        {
            // request 6 bytes from slave device
            Wire.requestFrom(I2CAddress, length);
            while ((Wire.available()) && (i < length)) // slave may send less than requested
            {
                c = Wire.read(); // receive a byte as character
                *outputPointer = c;
                outputPointer++;
                i++;
            }
        }
        break;

    case SPI_MODE:
        // take the chip select low to select the device:
        //digitalWrite(chipSelectPin, LOW);
        acellCs.write(Pin::State::low);
        // send the device the register you want to read:
        //SPI.transfer(offset | 0x80 | 0x40);  //Ored with "read request" bit and "auto increment" bit
        offset = offset | 0x80 | 0x40;
        HAL_SPI_Transmit(&hspi2, &offset, 1, HAL_MAX_DELAY);
        while (i < length) // slave may send less than requested
        {
            //c = SPI.transfer(0x00); // receive a byte as character
            HAL_SPI_Receive(&hspi2, &c, 1, HAL_MAX_DELAY);
            if (c == 0xFF) {
                //May have problem
                tempFFCounter++;
            }
            *outputPointer = c;
            outputPointer++;
            i++;
        }
        if (tempFFCounter == i) {
            //Ok, we've recieved all ones, report
            returnError = IMU_ALL_ONES_WARNING;
        }
        // take the chip select high to de-select:
        //digitalWrite(chipSelectPin, HIGH);
        acellCs.write(Pin::State::high);
        break;

    default:
        break;
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
status_t LIS3DHCore::readRegister(uint8_t *outputPointer, uint8_t offset) {
    //Return value
    uint8_t result;
    uint8_t numBytes = 1;
    status_t returnError = IMU_SUCCESS;

    switch (commInterface) {

    case I2C_MODE:
        Wire.beginTransmission(I2CAddress);
        Wire.write(offset);
        if (Wire.endTransmission() != 0) {
            returnError = IMU_HW_ERROR;
        }
        Wire.requestFrom(I2CAddress, numBytes);
        while (Wire.available()) // slave may send less than requested
        {
            result = Wire.read(); // receive a byte as a proper uint8_t
        }
        break;

    case SPI_MODE:
        // take the chip select low to select the device:
        //digitalWrite(chipSelectPin, LOW);
        acellCs.write(Pin::State::low);
        // send the device the register you want to read:
        //SPI.transfer(offset | 0x80);  //Ored with "read request" bit
        offset = offset | 0x80;
        HAL_SPI_Transmit(&hspi2, &offset, 1, HAL_MAX_DELAY);
        // send a value of 0 to read the first byte returned:
        //result = SPI.transfer(0x00);
        HAL_SPI_Receive(&hspi2, &result, 1, HAL_MAX_DELAY);
        // take the chip select high to de-select:
        //digitalWrite(chipSelectPin, HIGH);
        acellCs.write(Pin::State::high);

        if (result == 0xFF) {
            //we've recieved all ones, report
            returnError = IMU_ALL_ONES_WARNING;
        }
        break;

    default:
        break;
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
status_t LIS3DHCore::readRegisterInt16(int16_t *outputPointer, uint8_t offset) {
    {
        //offset |= 0x80; //turn auto-increment bit on
        uint8_t myBuffer[2];
        status_t returnError = readRegisterRegion(myBuffer, offset, 2); //Does memory transfer
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
status_t LIS3DHCore::writeRegister(uint8_t offset, uint8_t dataToWrite) {
    status_t returnError = IMU_SUCCESS;
    switch (commInterface) {
    case I2C_MODE:
        //Write the byte
        Wire.beginTransmission(I2CAddress);
        Wire.write(offset);
        Wire.write(dataToWrite);
        if (Wire.endTransmission() != 0) {
            returnError = IMU_HW_ERROR;
        }
        break;

    case SPI_MODE:
        // take the chip select low to select the device:
        //digitalWrite(chipSelectPin, LOW);
        acellCs.write(Pin::State::low);
        // send the device the register you want to read:
        //PI.transfer(offset);
        HAL_SPI_Transmit(&hspi2, &offset, 1, HAL_MAX_DELAY);
        // send a value of 0 to read the first byte returned:
        //SPI.transfer(dataToWrite);
        HAL_SPI_Transmit(&hspi2, &dataToWrite, 1, HAL_MAX_DELAY);
        // decrement the number of bytes left to read:
        // take the chip select high to de-select:
        //digitalWrite(chipSelectPin, HIGH);
        acellCs.write(Pin::State::high);
        break;

        //No way to check error on this write (Except to read back but that's not reliable)

    default:
        break;
    }

    return returnError;
}

//****************************************************************************//
//
//  Main user class -- wrapper for the core class + maths
//
//  Construct with same rules as the core ( uint8_t busType, uint8_t inputArg )
//
//****************************************************************************//
LIS3DH::LIS3DH(uint8_t busType, uint8_t inputArg)
    : LIS3DHCore(busType, inputArg) {
    //Construct with these default settings
    //ADC stuff
    settings.adcEnabled = 1;

    //Temperature settings
    settings.tempEnabled = 1;

    //Accelerometer settings
    settings.accelSampleRate = 1600; //Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
    settings.accelRange = 2;         //Max G force readable.  Can be: 2, 4, 8, 16

    settings.xAccelEnabled = 1;
    settings.yAccelEnabled = 1;
    settings.zAccelEnabled = 1;

    //FIFO control settings
    settings.fifoEnabled = 0;
    settings.fifoThreshold = 20; //Can be 0 to 32
    settings.fifoMode = 0;       //FIFO mode.

    allOnesCounter = 0;
    nonSuccessCounter = 0;
    isInicialized = false;
}

//****************************************************************************//
//
//  Begin
//
//  This starts the lower level begin, then applies settings
//
//****************************************************************************//
status_t LIS3DH::begin(void) {
    //Begin the inherited core.  This gets the physical wires connected
    status_t returnError = beginCore();

    applySettings();
    isInicialized = true;
    return returnError;
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
void LIS3DH::applySettings(void) {
    uint8_t dataToWrite = 0; //Temporary variable

    //Build TEMP_CFG_REG
    dataToWrite = 0; //Start Fresh!
    dataToWrite = ((settings.tempEnabled & 0x01) << 6) | ((settings.adcEnabled & 0x01) << 7);
    //Now, write the patched together data
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_TEMP_CFG_REG: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    writeRegister(LIS3DH_TEMP_CFG_REG, dataToWrite);

    //Build CTRL_REG1
    dataToWrite = 0; //Start Fresh!
    //  Convert ODR
    switch (settings.accelSampleRate) {
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
    case 5000:
        dataToWrite |= (0x09 << 4);
        break;
    }

    dataToWrite |= (settings.zAccelEnabled & 0x01) << 2;
    dataToWrite |= (settings.yAccelEnabled & 0x01) << 1;
    dataToWrite |= (settings.xAccelEnabled & 0x01);
    //Now, write the patched together data
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_CTRL_REG1: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    writeRegister(LIS3DH_CTRL_REG1, dataToWrite);

    //Build CTRL_REG4
    dataToWrite = 0; //Start Fresh!
    //  Convert scaling
    switch (settings.accelRange) {
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
    dataToWrite |= 0x80; //set block update
    dataToWrite |= 0x08; //set high resolution
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_CTRL_REG4: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    //Now, write the patched together data
    writeRegister(LIS3DH_CTRL_REG4, dataToWrite);
}
//****************************************************************************//
//
//  Accelerometer section
//
//****************************************************************************//
int16_t LIS3DH::readRawAccelX(void) {
    int16_t output;
    status_t errorLevel = readRegisterInt16(&output, LIS3DH_OUT_X_L);
    if (errorLevel != IMU_SUCCESS) {
        if (errorLevel == IMU_ALL_ONES_WARNING) {
            allOnesCounter++;
        } else {
            nonSuccessCounter++;
        }
    }
    return output;
}
float LIS3DH::readFloatAccelX(void) {
    float output = calcAccel(readRawAccelX());
    return output;
}

int16_t LIS3DH::readRawAccelY(void) {
    int16_t output;
    status_t errorLevel = readRegisterInt16(&output, LIS3DH_OUT_Y_L);
    if (errorLevel != IMU_SUCCESS) {
        if (errorLevel == IMU_ALL_ONES_WARNING) {
            allOnesCounter++;
        } else {
            nonSuccessCounter++;
        }
    }
    return output;
}

float LIS3DH::readFloatAccelY(void) {
    float output = calcAccel(readRawAccelY());
    return output;
}

int16_t LIS3DH::readRawAccelZ(void) {
    int16_t output;
    status_t errorLevel = readRegisterInt16(&output, LIS3DH_OUT_Z_L);
    if (errorLevel != IMU_SUCCESS) {
        if (errorLevel == IMU_ALL_ONES_WARNING) {
            allOnesCounter++;
        } else {
            nonSuccessCounter++;
        }
    }
    return output;
}

float LIS3DH::readFloatAccelZ(void) {
    float output = calcAccel(readRawAccelZ());
    return output;
}

float LIS3DH::calcAccel(int16_t input) {
    float output;
    switch (settings.accelRange) {
    case 2:
        output = (float)input / 15987;
        break;
    case 4:
        output = (float)input / 7840;
        break;
    case 8:
        output = (float)input / 3883;
        break;
    case 16:
        output = (float)input / 1280;
        break;
    default:
        output = 0;
        break;
    }
    return output;
}

//****************************************************************************//
//
//  Accelerometer section
//
//****************************************************************************//
uint16_t LIS3DH::read10bitADC1(void) {
    int16_t intTemp;
    uint16_t uintTemp;
    readRegisterInt16(&intTemp, LIS3DH_OUT_ADC1_L);
    intTemp = 0 - intTemp;
    uintTemp = intTemp + 32768;
    return uintTemp >> 6;
}

uint16_t LIS3DH::read10bitADC2(void) {
    int16_t intTemp;
    uint16_t uintTemp;
    readRegisterInt16(&intTemp, LIS3DH_OUT_ADC2_L);
    intTemp = 0 - intTemp;
    uintTemp = intTemp + 32768;
    return uintTemp >> 6;
}

uint16_t LIS3DH::read10bitADC3(void) {
    int16_t intTemp;
    uint16_t uintTemp;
    readRegisterInt16(&intTemp, LIS3DH_OUT_ADC3_L);
    intTemp = 0 - intTemp;
    uintTemp = intTemp + 32768;
    return uintTemp >> 6;
}

//****************************************************************************//
//
//  FIFO section
//
//****************************************************************************//
void LIS3DH::fifoBegin(void) {
    uint8_t dataToWrite = 0; //Temporary variable

    //Build LIS3DH_FIFO_CTRL_REG
    readRegister(&dataToWrite, LIS3DH_FIFO_CTRL_REG); //Start with existing data
    dataToWrite &= 0x20;                              //clear all but bit 5
    dataToWrite |= (settings.fifoMode & 0x03) << 6;   //apply mode
    dataToWrite |= (settings.fifoThreshold & 0x1F);   //apply threshold
                                                      //Now, write the patched together data
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);

    //Build CTRL_REG5
    readRegister(&dataToWrite, LIS3DH_CTRL_REG5); //Start with existing data
    dataToWrite &= 0xBF;                          //clear bit 6
    dataToWrite |= (settings.fifoEnabled & 0x01) << 6;
    //Now, write the patched together data
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_CTRL_REG5: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    writeRegister(LIS3DH_CTRL_REG5, dataToWrite);
}

void LIS3DH::fifoClear(void) {
    //Drain the fifo data and dump it
    while ((fifoGetStatus() & 0x20) == 0) {
        readRawAccelX();
        readRawAccelY();
        readRawAccelZ();
    }
}

void LIS3DH::fifoStartRec(void) {
    uint8_t dataToWrite = 0; //Temporary variable

    //Turn off...
    readRegister(&dataToWrite, LIS3DH_FIFO_CTRL_REG); //Start with existing data
    dataToWrite &= 0x3F;                              //clear mode
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);
    //  ... then back on again
    readRegister(&dataToWrite, LIS3DH_FIFO_CTRL_REG); //Start with existing data
    dataToWrite &= 0x3F;                              //clear mode
    dataToWrite |= (settings.fifoMode & 0x03) << 6;   //apply mode
                                                      //Now, write the patched together data
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);
}

uint8_t LIS3DH::fifoGetStatus(void) {
    //Return some data on the state of the fifo
    uint8_t tempReadByte = 0;
    readRegister(&tempReadByte, LIS3DH_FIFO_SRC_REG);
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_FIFO_SRC_REG: 0x");
    Serial.println(tempReadByte, HEX);
    #endif
    return tempReadByte;
}

void LIS3DH::fifoEnd(void) {
    uint8_t dataToWrite = 0; //Temporary variable

    //Turn off...
    readRegister(&dataToWrite, LIS3DH_FIFO_CTRL_REG); //Start with existing data
    dataToWrite &= 0x3F;                              //clear mode
    #ifdef VERBOSE_SERIAL
    Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
    Serial.println(dataToWrite, HEX);
    #endif
    writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);
}

bool LIS3DH::isSetupDone() {
    return isInicialized;
}
#endif
