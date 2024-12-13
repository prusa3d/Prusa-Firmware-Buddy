#include <device/board.h>
#include <option/has_mmu2.h>
#include <option/has_mmu2_over_uart.h>

#if not HAS_MMU2_OVER_UART()
    #include <puppies/xbuddy_extension.hpp>
    #include <freertos/timing.hpp>
#endif

#if HAS_MMU2()
    #include <buddy/mmu_port.hpp>

namespace MMU2 {

using namespace buddy::hw;

    #if HAS_MMU2_OVER_UART()

void power_on() {
    mmu_port::setup_reset_pin();
    mmu_port::power_on();
}

void power_off() {
    mmu_port::power_off();
}

void reset() {
    mmu_port::activate_reset();
    HAL_Delay(5);
    mmu_port::deactivate_reset();
    HAL_Delay(5);
}

    #else
void power_on() {
    buddy::puppies::xbuddy_extension.set_mmu_power(true);
}

void power_off() {
    buddy::puppies::xbuddy_extension.set_mmu_power(false);
}

void reset() {
    buddy::puppies::xbuddy_extension.set_mmu_nreset(false);
    freertos::delay(5);
    buddy::puppies::xbuddy_extension.set_mmu_nreset(true);
    freertos::delay(5);
}

    #endif

} // namespace MMU2
#endif
