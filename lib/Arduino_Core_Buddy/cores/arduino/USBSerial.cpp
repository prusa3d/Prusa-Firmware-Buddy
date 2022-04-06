#include "USBSerial.h"
#include "tusb.h"

void USBSerial::enable() {
    enabled = true;
}

void USBSerial::disable() {
    enabled = false;
    tud_cdc_write_clear();
    lineBufferUsed = 0;
}

void USBSerial::setIsWriteOnly(bool writeOnly) {
    isWriteOnly = writeOnly;
}

int USBSerial::available(void) {
    if (!enabled || isWriteOnly)
        return 0;

    return tud_cdc_available();
}

int USBSerial::peek(void) {
    if (!enabled || isWriteOnly)
        return 0;

    return tud_cdc_peek(0);
}

int USBSerial::read(void) {
    if (!enabled || isWriteOnly)
        return 0;

    return tud_cdc_read_char();
}

size_t USBSerial::readBytes(char *buffer, size_t length) {
    if (!enabled || isWriteOnly)
        return 0;

    return tud_cdc_read(buffer, length);
}

void USBSerial::flush(void) {
    if (enabled)
        tud_cdc_write_flush();

    if (lineBufferUsed) {
        lineBufferHook(&lineBuffer[0], lineBufferUsed);
        lineBufferUsed = 0;
    }
}

void USBSerial::LineBufferAppend(char character) {
    if (lineBufferUsed < (lineBuffer.size() - 3)) {
        lineBuffer[lineBufferUsed++] = character;
    } else if (lineBufferUsed == lineBuffer.size() - 3) {
        lineBuffer[lineBufferUsed++] = '.';
        lineBuffer[lineBufferUsed++] = '.';
    }
}

size_t USBSerial::write(uint8_t ch) {
    int written = enabled ? tud_cdc_write_char(ch) : 1;

    if (written) {
        LineBufferAppend(ch);
        if (ch == '\n')
            flush();
    }

    return written;
}

size_t USBSerial::write(const uint8_t *buffer, size_t size) {
    int written = enabled ? tud_cdc_write(buffer, size) : size;

    for (int remaining = written; remaining > 0; remaining--, buffer++) {
        LineBufferAppend(*buffer);
        if (*buffer == '\n')
            flush();
    }

    return written;
}

USBSerial::operator bool(void) {
    return true;
}

USBSerial SerialUSB = USBSerial();
