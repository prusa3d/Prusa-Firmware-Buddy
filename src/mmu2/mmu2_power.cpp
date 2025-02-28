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
    #include <freertos/critical_section.hpp>
    #include "hw_configuration.hpp"
    #if BOARD_IS_XBUDDY()
        #include <advanced_power.hpp>
    #endif

namespace MMU2 {

using namespace buddy::hw;

inline static void activate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::low : Pin::State::high);
}

inline static void deactivate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::high : Pin::State::low);
}

template <uint32_t us_high, uint32_t us_low, uint32_t us_total>
void mmu_soft_start() {
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

    // The code below pulse-charges the MMU capacitors, as the current inrush
    // would due to an inferior HW design cause overcurrent on the xBuddy board.
    // In case overcurrent would still be triggered, increase the us_total
    // value to pre-charge longer.
    if (config.needs_software_mmu_powerup()) {
        freertos::CriticalSection critical_section;
        if (!config.has_mmu_power_up_hw()) {
            static constexpr uint32_t us_high = 5;
            static constexpr uint32_t us_low = 70;
            static constexpr uint32_t us_total = 15000;
            mmu_soft_start<us_high, us_low, us_total>();
        } else {
            // 50 ms - 96%
            mmu_soft_start<480, 20, 50'000>();
            // 350 ms - dynamic
            // start at: 99.6%
            // if MMU_CURRENT > 1.1V -> 99%
            // elif MMU_CURRENT < 0.7V -> 99.6%
            static constexpr uint32_t us_high_99_0 = 495;
            static constexpr uint32_t us_low_99_0 = 5;
            static constexpr uint32_t us_high_98_0 = 490;
            static constexpr uint32_t us_low_98_0 = 10;
            static constexpr uint32_t us_total = 350'000;
            static constexpr uint32_t us_step = us_high_99_0 + us_low_99_0;
            uint32_t us_high = us_high_99_0;
            uint32_t us_low = us_low_99_0;

            for (uint32_t i = 0; i < us_total; i += us_step) {
                const auto mmu_current = advancedpower.GetMMUInputCurrent();
                if (us_high == us_high_99_0 && mmu_current > 1.1f) {
                    us_high = us_high_98_0;
                    us_low = us_low_98_0;
                } else if (us_high == us_high_98_0 && mmu_current < 0.7f) {
                    us_high = us_high_99_0;
                    us_low = us_low_99_0;
                }

                MMUEnable.write(Pin::State::high);
                delay_us(us_high);
                if ((i + us_step) < us_total) {
                    MMUEnable.write(Pin::State::low);
                    delay_us(us_low);
                }
            }
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
} // namespace MMU2
#endif
