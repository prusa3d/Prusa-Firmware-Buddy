#include <buddy/mmu_port.hpp>

#include <common/interrupt_disabler.hpp>
#include <common/timing_precise.hpp>
#include <common/timing.h>
#include <freertos/critical_section.hpp>
#include <buddy/mmu_port.hpp>
#include <common/hwio_pindef.h>
#include <hw_configuration.hpp>
#include <freertos/binary_semaphore.hpp>
#include <common/awdg.hpp>

namespace mmu_port {

// TODO: Make hw a standalone module -> this would allow this to be standalone module too
using namespace buddy::hw;

void activate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::low : Pin::State::high);
}

void deactivate_reset() {
    MMUReset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::high : Pin::State::low);
}

void setup_reset_pin() {
    const auto &config = Configuration::Instance();

    // Newer BOMs need push-pull for the reset pin, older open drain.
    // Setting it like this is a bit hacky, because the MMUReset defined in hwio_pindef is constexpr,
    // so it's not possible to change it right at the source.
    if (config.needs_push_pull_mmu_reset_pin()) {
        OutputPin pin = MMUReset;
        pin.m_mode = OMode::pushPull;
        pin.configure();
    }
}

template <uint32_t us_high, uint32_t us_low, uint32_t us_total>
void mmu_soft_start() {
    for (uint32_t i = 0; i < us_total; i += (us_high + us_low)) {
        {
            buddy::InterruptDisabler disable_interrupts;
            MMUEnable.write(Pin::State::high);
            delay_us_precise<us_high>();
            MMUEnable.write(Pin::State::low);
        }
        delay_us(us_low);
    }
}

void power_on() {
    const auto &config = Configuration::Instance();

    // Power on the MMU with sreset activated
    activate_reset();

    if (config.needs_software_mmu_powerup()) {
        if (!config.has_mmu_power_up_hw()) {
            // The code below pulse-charges the MMU capacitors, as the current inrush
            // would due to an inferior HW design cause overcurrent on the xBuddy board.
            // In case overcurrent would still be triggered, increase the us_total
            // value to pre-charge longer.
            freertos::CriticalSection critical_section;
            static constexpr uint32_t us_high = 5;
            static constexpr uint32_t us_low = 70;
            static constexpr uint32_t us_total = 15000;
            mmu_soft_start<us_high, us_low, us_total>();

            MMUEnable.write(Pin::State::high);

            // Give some time for the MMU to catch up with the reset signal - it takes some time for the voltage to actually start
            delay(200);
        } else {
            static constexpr uint16_t HIGH_THR = 2600; // 2500 is also working, but not 100% time, but that was tested before chargin speedup, so it might be Ok now.
            static constexpr uint16_t MAX_THR = 0xfff; // 12-bit adc max is 0xfff
            static constexpr uint32_t START_WAIT_TIME = 500; // wait time for log0 in us
            static constexpr int32_t MAX_CHARGE_TIME_BEFORE_SPEEDUP = 100'000; // total max wait time in us
            freertos::BinarySemaphore early_oc;
            volatile uint32_t trigger_time = 0;
            auto awdg = awdg::get_single_channel_watchdog(
                &hadc3, ADC_CHANNEL_4, [&trigger_time, &early_oc](awdg::ADCWatchdog &wdg) {
                wdg.adjust_range(0, MAX_THR);
                MMUEnable.reset();
                trigger_time = ticks_us();
                early_oc.release_from_isr(); }, 0, HIGH_THR);
            auto current_wait_time = START_WAIT_TIME;
            auto start_time = ticks_us();
            while (true) {
                awdg->adjust_range(0, HIGH_THR);
                MMUEnable.set();
                if (!early_oc.try_acquire_for(100)) {
                    // We didn't trigger the watchdog for 100ms, ether the MMU is not connected or the MMU is charged enough and we can continue
                    break;
                }
                const auto current_ticks = ticks_us();
                if (ticks_diff(current_ticks, start_time) >= MAX_CHARGE_TIME_BEFORE_SPEEDUP) {
                    // MMU wasn't charged in time the sequence is taking too long - lets try to lower the wait time to let more current into MMU
                    if (current_wait_time > 300) {
                        current_wait_time -= 100;
                    } else {
                        current_wait_time -= 10;
                    }
                    start_time = current_ticks;
                }

                // We should wait for current_wait_time until we turn the power on again.
                int32_t to_wait = ticks_diff(trigger_time + current_wait_time, current_ticks);
                if (to_wait > 0) {
                    delay_us(to_wait);
                }
            }
        }
    } else {
        MMUEnable.write(Pin::State::high);
    }

    deactivate_reset();
}

void power_off() {
    MMUEnable.write(Pin::State::low);
}

} // namespace mmu_port
