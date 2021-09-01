//USBSerial.h - A3ides/STM32
#ifndef _USBSERIAL_H
#define _USBSERIAL_H

#include "Arduino.h"
#include "Stream.h"

class USBSerial : public Stream {
public:
    void begin(uint32_t);
    virtual int available(void);
    virtual int availableForWrite(void);
    virtual int peek(void);
    virtual int read(void);
    virtual size_t readBytes(char *buffer, size_t length);                       // read chars from stream into buffer
    virtual size_t readBytesUntil(char terminator, char *buffer, size_t length); // as readBytes with terminator character
    virtual void flush(void);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buffer, size_t size);
    operator bool(void);
};

extern USBSerial SerialUSB;

#endif //_USBSERIAL_H
