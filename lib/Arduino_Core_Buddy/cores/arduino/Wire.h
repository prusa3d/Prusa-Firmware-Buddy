// Wire.h - Buddy/STM32
#ifndef _WIRE_H
#define _WIRE_H

#include <inttypes.h>
#include "Arduino.h"
#include "Stream.h"

class TwoWire : public Stream {
public:
    TwoWire(void);
    void begin(void);
    void beginTransmission(uint8_t);
    uint8_t endTransmission(void);
    uint8_t requestFrom(uint8_t, uint8_t);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *, size_t);
    virtual int available(void);
    virtual int read(void);
    virtual int peek(void);
    virtual void flush(void);
};

extern TwoWire Wire;

#endif //_WIRE_H
