#pragma once
#include <inttypes.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

bool w25x_communication_init(bool init_event_group);

void w25x_cs_low();
void w25x_cs_high();

void w25x_receive(uint8_t *buffer, uint32_t len);
uint8_t w25x_receive_byte();

void w25x_send(const uint8_t *buffer, uint32_t len);
void w25x_send_byte(uint8_t byte);

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)
