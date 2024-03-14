#pragma once

#include "Arduino.h"
#include "Stream.h"
#include <array>

class USBSerial : public Stream {
private:
    bool enabled;
    bool isWriteOnly;
    std::array<uint8_t, 128> lineBuffer;
    decltype(lineBuffer)::size_type lineBufferUsed;

    void LineBufferAppend(char character);

public:
    USBSerial()
        : enabled(false)
        , isWriteOnly(false)
        , lineBuffer()
        , lineBufferUsed(0) {}

    void enable();
    void disable();
    void setIsWriteOnly(bool writeOnly);
    void begin(uint32_t) {}
    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual size_t readBytes(char *buffer, size_t length);
    virtual void flush(void);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buffer, size_t size);
    operator bool(void);

    void (*lineBufferHook)(const uint8_t *buf, int len) { nullptr };
};

extern USBSerial SerialUSB;
