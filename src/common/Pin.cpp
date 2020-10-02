/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#include "Pin.hpp"
using namespace buddy::hw;

void InputPin::configure(Pull pull) const {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void OutputPin::configure() const {
    HAL_GPIO_WritePin(getHalPort(), m_halPin, static_cast<GPIO_PinState>(m_initState));
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Speed = static_cast<uint32_t>(m_speed);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void OutputInputPin::enableInput(Pull pull) const {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}
