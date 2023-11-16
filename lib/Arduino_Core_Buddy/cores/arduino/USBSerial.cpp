#include "USBSerial.h"
#include "tusb.h"
#include <task.h>

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
    if (!enabled || isWriteOnly) {
        return 0;
    }

    return tud_cdc_available();
}

int USBSerial::peek(void) {
    if (!enabled || isWriteOnly) {
        return 0;
    }

    return tud_cdc_peek(0);
}

int USBSerial::read(void) {
    if (!enabled || isWriteOnly) {
        return 0;
    }

    return tud_cdc_read_char();
}

size_t USBSerial::readBytes(char *buffer, size_t length) {
    if (!enabled || isWriteOnly) {
        return 0;
    }

    return tud_cdc_read(buffer, length);
}

void USBSerial::flush(void) {
    if (enabled) {
        tud_cdc_write_flush();
    }

    if (!lineBufferHook || !lineBufferUsed) {
        return;
    }

    lineBufferHook(&lineBuffer[0], lineBufferUsed);
    lineBufferUsed = 0;
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
    // its not possible to write to USB-CDC from ISR, so skip the write alltogether
    if (xPortIsInsideInterrupt()) {
        return 0;
    }

    if (enabled) {
        while (tud_cdc_write_char(ch) != 1) {
            // TX is full, yield to lower-priority (which usb is part of) threads until ready
            vTaskDelay(1);
        }
    }

    LineBufferAppend(ch);
    if (ch == '\n') {
        flush();
    }

    return 1;
}

static void cdc_write_sync(const uint8_t *buffer, size_t size) {
    for (;;) {
        size_t done = tud_cdc_write(buffer, size);
        if (done == size) {
            break;
        }

        // TX was full, yield to lower-priority (which usb is part of) threads until ready
        buffer += done;
        size -= done;
        vTaskDelay(1);
    }
}

size_t USBSerial::write(const uint8_t *buffer, size_t size) {
    // its not possible to write to USB-CDC from ISR, so skip the write alltogether
    if (xPortIsInsideInterrupt()) {
        return 0;
    }

    size_t beg = 0;
    size_t end = 0;

    for (end = 0; end != size; ++end) {
        uint8_t ch = buffer[end];
        LineBufferAppend(ch);
        if (ch == '\n') {
            // batch the last chunk up to the current \n
            cdc_write_sync(buffer + beg, end - beg + 1);
            beg = end + 1;
            flush();
        }
    }

    // complete the cdc write
    cdc_write_sync(buffer + beg, end - beg);
    return size;
}

USBSerial::operator bool(void) {
    return true;
}

USBSerial SerialUSB = USBSerial();
