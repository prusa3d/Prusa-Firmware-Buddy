#include "printers.h"
#include <device/board.h>
#include "timing_precise.hpp"

#if HAS_MMU2

    #include "inttypes.h"
    #include "hwio_pindef.h"
    #include "timing.h"
    #include "timing_precise.hpp"
    #include "disable_interrupts.h"
    #include "rtos_api.hpp"

namespace MMU2 {

using namespace buddy::hw;

static constexpr uint32_t us_high = 5;
static constexpr uint32_t us_low = 70;
static constexpr uint32_t us_total = 9000;

inline static void activate_reset() {
    #if (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 3, 4))
    // version == 0.34 (push-pull + mosfet)
    MMUReset.write(Pin::State::low);
    #else
    // version < 0.34 (open drain)
    MMUReset.write(Pin::State::high);
    #endif
}

inline static void deactivate_reset() {
    #if (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 3, 4))
    // version == 0.34 (push-pull + mosfet)
    MMUReset.write(Pin::State::high);
    #else
    // version < 0.34 (open drain)
    MMUReset.write(Pin::State::low);
    #endif
}

void power_on() {
    CriticalSection critical_section;
    activate_reset(); // Hold MMU procesor in reset state. Save power consume.
    for (uint32_t i = 0; i < us_total; i += (us_high + us_low)) {
        {
            buddy::DisableInterrupts disable_interrupts;
            MMUEnable.write(Pin::State::high);
            DELAY_US_PRECISE(us_high);
            MMUEnable.write(Pin::State::low);
        }
        delay_ns(us_low * 1000);
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
