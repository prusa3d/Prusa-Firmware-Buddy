#pragma once

#include "lwesp/lwesp.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void handle_rx_data(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif /* __cplusplus */
