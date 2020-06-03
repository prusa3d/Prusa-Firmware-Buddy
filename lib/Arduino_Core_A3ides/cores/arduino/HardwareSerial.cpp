//HardwareSerial.cpp - A3ides/STM32
#include <Arduino.h>
#include "cmsis_os.h"

// This include creates a dependency on the `Common` library.
// TODO: Create proper indirection.
//#include "dbg.h"

// uart2 (communication with TMC2209)

//#define DBG _dbg0 //debug level 0
#define DBG(...) //disable debug

extern "C" {

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern osThreadId uart2_thread;
extern int32_t uart2_signal;

uint8_t rbuff[10];
unsigned int rbufc = 0;
unsigned int rbufi = 0;
}

HardwareSerial::HardwareSerial(void *peripheral) {
}

void HardwareSerial::begin(unsigned long baud) {
}

void HardwareSerial::begin(unsigned long baud, byte config) {
}

int HardwareSerial::available(void) {
    return rbufc;
}

int HardwareSerial::peek(void) {
    return -1;
}

int HardwareSerial::read(void) {
    int ch = -1;
    if (rbufc) {
        ch = rbuff[rbufi++];
        rbufc--;
    }
    if (ch == -1)
        DBG(" rx -1");
    else
        DBG(" rx %02hhx %d", (uint8_t)ch, rbufc);
    return ch;
}

void HardwareSerial::flush() {
}

size_t HardwareSerial::write(uint8_t c) {
    static int cnt = 0;
    static uint8_t buf[10];
    uint8_t buf2[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int len = 4;
    buf[cnt++] = c;
    if ((cnt > 2) && (buf[2] & 0x80))
        len = 8;
    if (cnt >= len) {
        uart2_thread = osThreadGetId();
        int retry = 3;
        while (retry--) {
            if (len == 4)
                HAL_UART_Receive_DMA(&huart2, buf2, 12);
            else if (len == 8)
                HAL_UART_Receive_DMA(&huart2, buf2, 4);
            HAL_UART_Transmit(&huart2, buf, len, HAL_MAX_DELAY);
            if (len == 4) //
            {
                DBG("tx %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3]);
                osEvent ose;
                if ((ose = osSignalWait(uart2_signal, 100)).status == osEventSignal) {
                    DBG("signal, received %d", 12);
                    DBG("rx %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", buf2[0], buf2[1], buf2[2], buf2[3], buf2[4], buf2[5], buf2[6], buf2[7], buf2[8], buf2[9], buf2[10], buf2[11]);
                    memcpy(rbuff, buf2 + 4, 8);
                    rbufi = 0;
                    rbufc = 8;
                    retry = 0;
                } else if (ose.status == osEventTimeout) {
                    DBG("timeout, received %d", hdma_usart2_rx.Instance->NDTR);
                    HAL_UART_AbortReceive(&huart2);
                }
            } else if (len == 8) //
            {
                DBG("tx %02x %02x %02x %02x %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
                osEvent ose;
                if ((ose = osSignalWait(uart2_signal, 100)).status == osEventSignal) {
                    DBG("signal, received %d", 4);
                    DBG("rx %02x %02x %02x %02x", buf2[0], buf2[1], buf2[2], buf2[3]);
                    memcpy(rbuff, buf2, 8);
                    rbufi = 0;
                    rbufc = 4;
                    retry = 0;
                } else if (ose.status == osEventTimeout) {
                    DBG("timeout, received %d", hdma_usart2_rx.Instance->NDTR);
                    HAL_UART_AbortReceive(&huart2);
                }
            } else {
                DBG("error - (len!=4) && (len!=8)");
                HAL_UART_AbortReceive(&huart2);
            }
        }
        cnt = 0;
        uart2_thread = 0;
    }
    return 1;
}

HardwareSerial::operator bool() {
    return true;
}

HardwareSerial Serial3(USART3);
