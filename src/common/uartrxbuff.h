//uartrxbuff.h
#pragma once

#include <inttypes.h>
#include "stm32f4xx_hal.h"

typedef struct _uartrxbuff_t {
    UART_HandleTypeDef *phuart;
    DMA_HandleTypeDef *phdma;
    uint8_t *pdata;
    uint8_t flags;
    uint8_t index;
    uint8_t size;
    uint8_t reserve;
} uartrxbuff_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void uartrxbuff_init(uartrxbuff_t *prxbuff, UART_HandleTypeDef *phuart, DMA_HandleTypeDef *phdma, uint8_t size, uint8_t *pdata);

extern void uartrxbuff_open(uartrxbuff_t *prxbuff);

extern void uartrxbuff_close(uartrxbuff_t *prxbuff);

extern void uartrxbuff_reset(uartrxbuff_t *prxbuff);

extern int uartrxbuff_getchar(uartrxbuff_t *prxbuff);

extern void uartrxbuff_rxhalf_cb(uartrxbuff_t *prxbuff);

extern void uartrxbuff_rxcplt_cb(uartrxbuff_t *prxbuff);

#ifdef __cplusplus
}
#endif //__cplusplus
