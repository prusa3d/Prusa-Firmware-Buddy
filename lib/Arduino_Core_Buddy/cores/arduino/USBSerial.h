#pragma once

#include "Arduino.h"
#include "Stream.h"
#include <array>

class USBSerial : public Stream {
private:
    bool enabled;
    bool isWriteOnly;
    // Per-line capture buffer fed by every char written to USB. Used by
    // the lineBufferHook callback (e.g. WUI's /api/v1/log serial capture).
    // 128 B was too small for several common Marlin lines: M115's
    // FIRMWARE_NAME response is ~240 chars, full M503 settings dumps can
    // exceed 200, and M118 user messages have no documented cap. Anything
    // over the buffer was truncated to "..\n" in the hook output with no
    // way for downstream tooling to recover the rest of the line.
    // 512 B comfortably fits M115/M503/typical M118 with margin; the
    // ".." truncation marker still triggers if a line happens to exceed.
    std::array<uint8_t, 512> lineBuffer;
    decltype(lineBuffer)::size_type lineBufferUsed;
    static constexpr int32_t writeTimeoutUs = 3'000'000;

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

    // cdc write handlers
    void write_timeout(const int32_t us);
    void cdc_write_sync(const uint8_t *buffer, size_t size);

    void (*lineBufferHook)(const uint8_t *buf, int len) { nullptr };
};

extern USBSerial SerialUSB;
