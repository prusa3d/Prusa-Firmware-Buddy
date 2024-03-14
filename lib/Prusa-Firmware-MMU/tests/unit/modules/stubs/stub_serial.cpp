#include "serial.h"
#include "stub_serial.h"

namespace modules {
namespace serial {

SerialBuff tx;
SerialBuff rx;

bool WriteToUSART(const uint8_t *src, uint8_t len) {
    std::copy(src, src + len, std::back_inserter(tx));
    return true;
}

bool Available() {
    return !rx.empty();
}

uint8_t ConsumeByte() {
    if (rx.empty())
        return 0xff;
    uint8_t rv = rx.front();
    rx.erase(0);
    return rv;
}

void ClearRX() {
    rx.clear();
}

void ClearTX() {
    tx.clear();
}

} // namespace serial
} // namespace modules
