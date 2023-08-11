#include "printers.h"
#include <device/board.h>
#include "timing_precise.hpp"
#include <option/has_mmu2.h>

#if HAS_MMU2()

    #include "inttypes.h"
    #include "hwio_pindef.h"
    #include "timing.h"
    #include "timing_precise.hpp"
    #include "disable_interrupts.h"
    #include "rtos_api.hpp"
    #include "hw_configuration.hpp"

namespace MMU2 {

using namespace buddy::hw;

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
    activate_reset(); // Hold MMU procesor in reset state. Save power consume.
    if (!Configuration::Instance().can_power_up_mmu_without_pulses()) {
        CriticalSection critical_section;

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
} // namespace MMU2
#endif
