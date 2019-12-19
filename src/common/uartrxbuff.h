//uartrxbuff.h
#ifndef _UARTRXBUFF_H
#define _UARTRXBUFF_H

#include <inttypes.h>
#include "stm32f4xx_hal.h"

#define UARTRXBUFF_FLG_HALF 0x01
#define UARTRXBUFF_FLG_CPLT 0x02
#define UARTRXBUFF_FLG_FULL 0x04
#define UARTRXBUFF_FLG_OVER 0x08

#pragma pack(push)
#pragma pack(1)

typedef struct _uartrxbuff_t {
    UART_HandleTypeDef *phuart;
    DMA_HandleTypeDef *phdma;
    uint8_t flags;
    uint8_t index;
    uint8_t size;
    uint8_t *pdata;
} uartrxbuff_t;

#pragma pack(pop)

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

#endif //_UARTRXBUFF_H
