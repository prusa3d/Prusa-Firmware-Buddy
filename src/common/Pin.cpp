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

void InputPinGenericConfigure(uint16_t ioPin, IMode iMode, Pull pull, IoPort ioPort) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = ioPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(iMode);
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(IoPortToHal(ioPort), &GPIO_InitStruct);
}
