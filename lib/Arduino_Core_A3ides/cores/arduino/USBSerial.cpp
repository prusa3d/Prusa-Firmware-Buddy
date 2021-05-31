//USBSerial.cpp - A3ides/STM32

#include "USBSerial.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "usbd_def.h"

#define USBSERIAL_OBUF_SIZE 256
#define USBSERIAL_IBUF_SIZE 256
#define USBSERIAL_RETRY     100
#define USBSERIAL_MAX_FAIL  10

extern "C" {

extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);
extern int usbd_cdc_initialized;

int USBSerial_failcount = 0;

uint8_t obuff[USBSERIAL_OBUF_SIZE]; //output buffer
uint32_t obufc = 0;                 //number of characters in output buffer

uint8_t ibuff[USBSERIAL_IBUF_SIZE]; //input buffer
uint32_t ibufc = 0;                 //number of characters in input buffer
uint32_t ibufr = 0;                 //input buffer read index
uint32_t ibufw = 0;                 //input buffer write index

void usb_cdc_tx_buffer(void) {
    uint8_t ret;
    int retry = USBSERIAL_RETRY;
    if (hUsbDeviceFS.pClassData && usbd_cdc_initialized) // end-point connected
    {
        while (retry) {
            ret = CDC_Transmit_FS(obuff, obufc);
            if (ret == USBD_OK)
                break;
            osDelay(1);
            retry--;
        }
        if (retry == 0) {
            USBSerial_failcount++;
            if (USBSerial_failcount > USBSERIAL_MAX_FAIL) { //disable usb_cdc, reset fail counter
                usbd_cdc_initialized = 0;
                USBSerial_failcount = 0;
            }
        }
    }
    obufc = 0;
}
}

void USBSerial_put_rx_data(uint8_t *buffer, uint32_t length) {
    if ((USBSERIAL_IBUF_SIZE - ibufc) >= length) {
        while (length--) {
            ibuff[ibufw++] = *buffer;
            if (ibufw >= USBSERIAL_IBUF_SIZE)
                ibufw = 0;
            buffer++;
            ibufc++; //is atomic
        }
    } else {
        //input buffer overflow
    }
}

void USBSerial::begin(uint32_t baud_count) {
    // uart config is ignored in USB-CDC
}

int USBSerial::available(void) {
    return ibufc;
}

int USBSerial::availableForWrite(void) {
    return (USBSERIAL_OBUF_SIZE - obufc);
}

int USBSerial::peek(void) {
    int ch = -1;
    if (ibufc)
        ch = ibuff[ibufr];
    return ch;
}

int USBSerial::read(void) {
    int ch = -1;
    if (ibufc) {
        ch = ibuff[ibufr++];
        ibufc--;
        if (ibufr >= USBSERIAL_IBUF_SIZE)
            ibufr = 0;
    }
    return ch;
}

size_t USBSerial::readBytes(char *buffer, size_t length) {
    char ch;
    size_t readed = 0;
    while (ibufc && length) {
        ch = ibuff[ibufr++];
        buffer[readed++] = ch;
        ibufc--;
        length--;
        if (ibufr >= USBSERIAL_IBUF_SIZE)
            ibufr = 0;
    }
    return readed;
}

size_t USBSerial::readBytesUntil(char terminator, char *buffer, size_t length) {
    char ch;
    size_t readed = 0;
    while (ibufc && length) {
        ch = ibuff[ibufr++];
        buffer[readed++] = ch;
        ibufc--;
        length--;
        if (ibufr >= USBSERIAL_IBUF_SIZE)
            ibufr = 0;
        if (ch == terminator)
            break;
    }
    return readed;
}

void USBSerial::flush(void) {
    if (obufc)
        usb_cdc_tx_buffer();
}

size_t USBSerial::write(uint8_t ch) {
    if (obufc < USBSERIAL_OBUF_SIZE) {
        obuff[obufc++] = ch;
    }
    if ((ch == '\n') || (obufc == USBSERIAL_OBUF_SIZE)) // eoln or full
        usb_cdc_tx_buffer();
    return 1;
}

size_t USBSerial::write(const uint8_t *buffer, size_t size) {
    size_t written = 0;
    while (size--) {
        if (write(*buffer) != 1)
            break;
        written++;
        buffer++;
    }
    return written;
}

USBSerial::operator bool(void) {
    return true;
}

USBSerial SerialUSB = USBSerial();
