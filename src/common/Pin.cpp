/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#include "Pin.hpp"

/**
 * @brief Points to last created object of this class, nullptr for none
 */
ConfigurableIndestructible *ConfigurableIndestructible::s_last = nullptr;

/**
 * @brief Configure all instances of objects derived from ConfigurableIndestructible
 *
 * Objects are configured in opposite order as it was created.
 */
void ConfigurableIndestructible::configure_all() {
    ConfigurableIndestructible *linkedListItem = s_last;
    for (; linkedListItem != nullptr; linkedListItem = linkedListItem->m_previous) {
        linkedListItem->configure();
    }
}

void InputPin::configure() {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Pull = static_cast<uint32_t>(m_pull);
    HAL_GPIO_Init(m_halPort, &GPIO_InitStruct);
}

void OutputPin::configure() {
    HAL_GPIO_WritePin(m_halPort, m_halPin, static_cast<GPIO_PinState>(m_initState));
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Speed = static_cast<uint32_t>(m_speed);
    HAL_GPIO_Init(m_halPort, &GPIO_InitStruct);
}

void OutputInputPin::enableInput(Pull pull) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(m_halPort, &GPIO_InitStruct);
}
