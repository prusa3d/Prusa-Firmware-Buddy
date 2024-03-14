#include "serial.h"
#include "../hal/usart.h"

namespace modules {

namespace serial {

bool WriteToUSART(const uint8_t *src, uint8_t len) {
    // How to properly enqueue the message? Especially in case of a full buffer.
    // We neither can stay here in an endless loop until the buffer drains.
    // Nor can we save the message elsewhere ... it must be just skipped and the protocol must handle it.
    // Under normal circumstances, such a situation should not happen.
    // The MMU cannot produce response messages on its own - it only responds to requests from the printer.
    // That means there is only one message in the output buffer at once as long as the printer waits for the response before sending another request.
    for (uint8_t i = 0; i < len; ++i) {
        if (hu::usart1.CanWrite()) {
            // should not block waiting for the TX buffer to drain as there was an empty spot for at least 1 byte
            hu::usart1.Write(src[i]);
        } else {
            //buffer full - must skip the rest of the message - the communication will drop out anyway
            return false;
        }
    }
    return true; // not sure if we can actually leverage the knowledge of success while sending the message
}

bool Available() {
    return !hu::usart1.ReadEmpty();
}

uint8_t ConsumeByte() {
    return hu::usart1.Read();
}

} // namespace serial

} // namespace modules
