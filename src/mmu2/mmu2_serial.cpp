#include <Marlin/src/feature/prusa/MMU2/mmu2_serial.h>

#include <common/timing.h>
#include <device/hal.h>
#include <device/peripherals_uart.hpp>
#include <option/has_mmu2.h>

namespace MMU2 {

#if HAS_PUPPIES() and HAS_MMU2()
// @@TODO temporary empty implementation just to see what else needs to be split
void MMU2Serial::begin() {
}

void MMU2Serial::close() {
}

int MMU2Serial::read() {
    return -1;
}

void MMU2Serial::flush() {
}

size_t MMU2Serial::write(const uint8_t * /*buffer*/, size_t /*size*/) {
    return 0;
}

void MMU2Serial::check_recovery() {
}

MMU2Serial mmu2Serial;

#elif HAS_MMU2_OVER_UART()

void MMU2Serial::begin() {
    uart_for_mmu.Open();
    // zero the default read timeout to make BufferedSerial::Read() non-blocking
    uart_for_mmu.SetReadTimeoutMs(0);
}

void MMU2Serial::close() {
    uart_for_mmu.Close();
}

int MMU2Serial::read() {
    char ch;
    int read = uart_for_mmu.Read(&ch, 1);
    return read ? ch : -1;
}

void MMU2Serial::flush() {
    uart_for_mmu.Flush();
}

size_t MMU2Serial::write(const uint8_t *buffer, size_t size) {
    return uart_for_mmu.Write((const char *)buffer, size);
}

void MMU2Serial::check_recovery() {
    // Some xBuddy BOMs have a pulldown on the USART RX.
    // This is causing frame errors when the MMU is rebooting and such.
    // On error, USART_CR3_DMAR gets disabled (DMA enable read) from UART_EndRxTransfer.
    // Wait till the RX pin gets to high again and then reset the serial.
    // BFW-5286
    if (USART6->CR3 & USART_CR3_DMAR) {
        return;
    }

    const auto now = ticks_ms();
    if (!recovery_start_ms || HAL_GPIO_ReadPin(MMU_RX_GPIO_Port, MMU_RX_Pin) != GPIO_PIN_SET) {
        recovery_start_ms = now;

    } else if (ticks_diff(now, recovery_start_ms) > 50) {
        close();
        begin();
    }
}

MMU2Serial mmu2Serial;

#endif

} // namespace MMU2
