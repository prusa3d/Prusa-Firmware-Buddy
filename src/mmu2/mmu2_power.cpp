#include "printers.h"
#include <device/board.h>
#include "timing_precise.hpp"
#include <option/has_mmu2.h>
#include <option/has_mmu2_over_uart.h>

#if not HAS_MMU2_OVER_UART()
    #include <puppies/xbuddy_extension.hpp>
    #include <freertos/timing.hpp>
#endif

#if HAS_MMU2()

    #include "inttypes.h"
    #include "hwio_pindef.h"
    #include "timing.h"
    #include "timing_precise.hpp"
    #include "disable_interrupts.h"
    #include <freertos/critical_section.hpp>
    #include "hw_configuration.hpp"

namespace MMU2 {

using namespace buddy::hw;

    #if HAS_MMU2_OVER_UART()

// The code below pulse-charges the MMU capacitors, as the current inrush
// would due to an inferior HW design cause overcurrent on the xBuddy board.
// In case overcurrent would still be triggered, increase the us_total
// value to pre-charge longer.
static constexpr uint32_t us_high = 5;
static constexpr uint32_t us_low = 70;
static constexpr uint32_t us_total = 15000;

inline static void activate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::low : Pin::State::high);
}

inline static void deactivate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::high : Pin::State::low);
}

void power_on() {
    const auto &config = Configuration::Instance();

    // Newer BOMs need push-pull for the reset pin, older open drain.
    // Setting it like this is a bit hacky, because the MMUReset defined in hwio_pindef is constexpr,
    // so it's not possible to change it right at the source.
    if (config.needs_push_pull_mmu_reset_pin()) {
        OutputPin pin = MMUReset;
        pin.m_mode = OMode::pushPull;
        pin.configure();
    }

    // Power on the MMU with sreset activated
    activate_reset();

    if (!config.can_power_up_mmu_without_pulses()) {
        freertos::CriticalSection critical_section;

        for (uint32_t i = 0; i < us_total; i += (us_high + us_low)) {
            {
                buddy::DisableInterrupts disable_interrupts;
                MMUEnable.write(Pin::State::high);
                delay_us_precise<us_high>();
                MMUEnable.write(Pin::State::low);
            }
            delay_us(us_low);
        }
    }

    MMUEnable.write(Pin::State::high);

    // Give some time for the MMU to catch up with the reset signal - it takes some time for the voltage to actually start
    delay(200);

    deactivate_reset();
}

void power_off() {
    MMUEnable.write(Pin::State::low);
}

void reset() {
    activate_reset();
    HAL_Delay(5);
    deactivate_reset();
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
