// uartrxbuff.c
#include "uartrxbuff.h"
#include "dbg.h"

void uartrxbuff_rx_full(uartrxbuff_t *prxbuff) {
    prxbuff->flags |= UARTRXBUFF_FLG_FULL;
    _dbg0("uartrxbuff_rx_full");
}

void uartrxbuff_rx_overflow(uartrxbuff_t *prxbuff) {
    prxbuff->flags |= UARTRXBUFF_FLG_OVER;
    _dbg0("uartrxbuff_rx_overflow");
}

void uartrxbuff_init(uartrxbuff_t *prxbuff, UART_HandleTypeDef *phuart, DMA_HandleTypeDef *phdma, uint8_t size, uint8_t *pdata) {
    prxbuff->phuart = phuart;
    prxbuff->phdma = phdma;
    prxbuff->flags = 0;
    prxbuff->index = 0;
    prxbuff->size = size;
    prxbuff->pdata = pdata;
}

void uartrxbuff_open(uartrxbuff_t *prxbuff) {
    HAL_UART_Receive_DMA(prxbuff->phuart, prxbuff->pdata, prxbuff->size);
    uartrxbuff_reset(prxbuff);
}

void uartrxbuff_close(uartrxbuff_t *prxbuff) {
    HAL_UART_AbortReceive(prxbuff->phuart);
}

void uartrxbuff_reset(uartrxbuff_t *prxbuff) {
    prxbuff->flags = 0;
    prxbuff->index = prxbuff->size - prxbuff->phdma->Instance->NDTR;
}

int uartrxbuff_getchar(uartrxbuff_t *prxbuff) {
    int ch = -1;
    uint32_t ndtr = prxbuff->phdma->Instance->NDTR;
    uint8_t cnt = prxbuff->size - ndtr;
    if (prxbuff->index < (prxbuff->size / 2)) {
        if ((prxbuff->flags & UARTRXBUFF_FLG_HALF) || (prxbuff->index < cnt)) {
            ch = prxbuff->pdata[prxbuff->index++];
            if (prxbuff->index == (prxbuff->size / 2))
                prxbuff->flags &= ~UARTRXBUFF_FLG_HALF;
        }
    } else {
        if ((prxbuff->flags & UARTRXBUFF_FLG_CPLT) || (prxbuff->index < cnt)) {
            ch = prxbuff->pdata[prxbuff->index++];
            if (prxbuff->index >= prxbuff->size) {
                prxbuff->index = 0;
                prxbuff->flags &= ~UARTRXBUFF_FLG_CPLT;
            }
        }
    }
    //TODO: check overflow
    return ch;
}

void uartrxbuff_rxhalf_cb(uartrxbuff_t *prxbuff) {
    if (prxbuff->flags & UARTRXBUFF_FLG_HALF)
        uartrxbuff_rx_overflow(prxbuff);
    else {
        prxbuff->flags |= UARTRXBUFF_FLG_HALF;
        if ((prxbuff->flags & UARTRXBUFF_FLG_CPLT) && (prxbuff->index == (prxbuff->size / 2)))
            uartrxbuff_rx_full(prxbuff);
    }
}

void uartrxbuff_rxcplt_cb(uartrxbuff_t *prxbuff) {
    if (prxbuff->flags & UARTRXBUFF_FLG_CPLT)
        uartrxbuff_rx_overflow(prxbuff);
    else {
        prxbuff->flags |= UARTRXBUFF_FLG_CPLT;
        if ((prxbuff->flags & UARTRXBUFF_FLG_HALF) && (prxbuff->index == 0))
            uartrxbuff_rx_full(prxbuff);
    }
}
