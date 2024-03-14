#include "printers.h"
#include <device/board.h>
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "buffered_serial.hpp"
    #include "cmsis_os.h"
    #include "bsod.h"
    #include "../common/hwio_pindef.h"

using namespace buddy::hw;

    #include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_serial.h"

namespace MMU2 {

    #define uartNr uart6

// @@TODO not available in BufferedSerial
// bool MMU2Serial::available()const {
//    return BufferedSerial::uartNr.Available();
//}

void MMU2Serial::begin([[maybe_unused]] uint32_t baud) {
    BufferedSerial::uartNr.Open();
    // zero the default read timeout to make BufferedSerial::Read() non-blocking
    BufferedSerial::uartNr.SetReadTimeoutMs(0);
}

void MMU2Serial::close() {
    BufferedSerial::uartNr.Close();
}

int MMU2Serial::read() {
    char ch;
    int read = BufferedSerial::uartNr.Read(&ch, 1);
    return read ? ch : -1;
}

void MMU2Serial::flush() {
    BufferedSerial::uartNr.Flush();
}

/// Generally, the xBuddy uses a full-duplex UART6 connected to an RS-485 converter.
/// The direction of communication is handled by a separate pin PB7
/// (low = receive, high = transmit from the xBuddy board's perspective)
/// As this direction pin is not HW-tied to the UART6's ability to switch directions in half-duplex mode
/// (because as noted above - the UART is running in full-duplex), we need to set the direction flag by hand in the FW.
/// Setting the direction to transmit is done here, setting to receive is done in main.cpp HAL_UART_TxCpltCallback
size_t MMU2Serial::write(const uint8_t *buffer, size_t size) {
    // set RS485 transmit direction - from xBuddy into the MMU
    //    RS485FlowControl.write(Pin::State::low); // beware of setting this to high when using normal RS-232! It will stop working arbitrarily
    // set to high in hwio_pindef.h
    // RS485FlowControl.write(Pin::State::high); // @@TODO for RS-232
    return BufferedSerial::uartNr.Write((const char *)buffer, size);
}

MMU2Serial mmu2Serial;

} // namespace MMU2

#else // HAS_MMU2()

    // a crude workaround to link FW for older incarnations of MK4's xBUDDY
    // Empty implementation for the MMU2Serial. Hopefully, with the release of MK4 this can be removed completely...
    #include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_serial.h"
namespace MMU2 {
void MMU2Serial::begin(uint32_t /*baud*/) {}
void MMU2Serial::close() {}
int MMU2Serial::read() { return 0; }
void MMU2Serial::flush() {}
size_t MMU2Serial::write(const uint8_t * /*buffer*/, size_t /*size*/) { return 0; }
MMU2Serial mmu2Serial;
} // namespace MMU2

#endif // HAS_MMU2()
