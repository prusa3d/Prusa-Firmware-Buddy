/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#include "Pin.hpp"

LinkedListItem *LinkedListItem::s_last = nullptr;

void LinkedListItem::configure_all() {
    LinkedListItem *linkedListItem = s_last;
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
