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
