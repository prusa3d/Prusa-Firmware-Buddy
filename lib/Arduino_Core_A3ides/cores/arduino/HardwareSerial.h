//HardwareSerial.h - A3ides/STM32
#ifndef _HARDWARESERIAL_H
#define _HARDWARESERIAL_H

#include <inttypes.h>
#include "Stream.h"

// Changes the serial output to UART1 TX/RX.
// Note you also need to change the marlin serial
// port from -1 to 1 in the marlin config.
//#define USE_UART1_SERIAL

class HardwareSerial : public Stream {
public:
    HardwareSerial(void *peripheral);
    void begin(unsigned long baud);
    void begin(unsigned long, uint8_t);
    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buffer, size_t size);
    operator bool();
};

#ifdef USE_UART1_SERIAL
// Route to USART1 instead of USB
class HardwareSerial2 : public Stream {
public:
    HardwareSerial2(void *p) {};
    void begin(unsigned long baud);
    virtual int available(void);
    virtual int peek(void) { return -1; };
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);
    virtual size_t write(uint8_t *buffer, size_t size);
    operator bool() { return true; };
};
extern HardwareSerial2 SerialUART1;
#endif

extern HardwareSerial Serial3;

#endif //_HARDWARESERIAL_H
