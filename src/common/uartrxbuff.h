#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include <inttypes.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"

#define UARTRXBUFF_ERR_NO_DATA  -1
#define UARTRXBUFF_ERR_OVERFLOW -2

enum {
    UARTRXBUFF_EVT_FIRST_HALF_FULL = (1 << 0),
    UARTRXBUFF_EVT_SECOND_HALF_FULL = (1 << 1),
    UARTRXBUFF_EVT_OVERFLOW_DETECTED = (1 << 2),
    UARTRXBUFF_EVT_IDLE = (1 << 3),
};

typedef struct _uartrxbuff_t {
    DMA_HandleTypeDef *phdma;

    /// Event group used to synchronize reading the buffer with DMA/UART interrupts
    EventGroupHandle_t event_group;

    /// Pointer to the buffer's memory itself
    uint8_t *buffer;

    /// Size of the buffer
    int buffer_size;

    /// Index of the next position in the buffer to read from
    int buffer_pos;
} uartrxbuff_t;

extern void uartrxbuff_init(uartrxbuff_t *prxbuff, UART_HandleTypeDef *phuart, DMA_HandleTypeDef *phdma, int size, uint8_t *pdata);

extern void uartrxbuff_reset(uartrxbuff_t *prxbuff);

extern int uartrxbuff_getchar(uartrxbuff_t *prxbuff);

extern void uartrxbuff_wait_for_event(uartrxbuff_t *prxbuff, uint32_t timeout);

extern void uartrxbuff_rxhalf_cb(uartrxbuff_t *prxbuff);

extern void uartrxbuff_rxcplt_cb(uartrxbuff_t *prxbuff);

extern void uartrxbuff_idle_cb(uartrxbuff_t *prxbuff);

#ifdef __cplusplus
}
#endif //__cplusplus
