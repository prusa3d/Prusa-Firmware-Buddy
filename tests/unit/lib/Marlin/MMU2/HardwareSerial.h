/// Stub for HardwareSerial.h from Marlin
/// gets compiled/included instead of the Marlin's one due to include path in the CMakeList.txt set to "."

#pragma once
#include <stdint.h>
#include <stddef.h>

class HardwareSerial {
public:
    HardwareSerial() = default;
    void begin(unsigned long baud);
    void close();
    virtual int read(void);
    virtual size_t write(const uint8_t *buffer, size_t size);
    void flush();
};

extern HardwareSerial Serial3;
