#pragma once

#include <cstdint>

namespace hal::RS485Driver {

typedef void (*OnReceiveCallback)(uint8_t *pData, uint32_t dataSize);

bool Init(uint8_t modbusAddress);
void SetOnReceiveCallback(OnReceiveCallback pCallback);

void StartReceiving();
bool Transmit(uint8_t *pData, uint32_t dataSize);

void DMA_IRQHandler();
void USART_IRQHandler();

} // namespace hal::RS485Driver
