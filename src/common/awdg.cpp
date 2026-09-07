#include "awdg.hpp"

#ifdef STM32F4

    #include <array>
    #include <bsod.h>
    #include <freertos/mutex.hpp>
    #include <device/peripherals.hpp>
    #include <stm32f4xx_hal_adc.h>

namespace awdg {
class F4ADCWatchdog : public awdg::ADCWatchdog {
public:
    F4ADCWatchdog(ADC_HandleTypeDef *hadc)
        : hadc(hadc) {}

    void adjust_range(uint16_t low, uint16_t high) final {
        hadc->Instance->HTR = high;
        hadc->Instance->LTR = low;
    }

    void release() final {
        // Invalidate range
        adjust_range(0, 0xfff);
        // Disable watchdog interupt
        // Normally enabled by HAL_ADC_AnalogWDGConfig
        __HAL_ADC_DISABLE_IT(hadc, ADC_IT_AWD);
        // Unlock mutex
        mtx.unlock();
    }

    void change_callback(awdg::ADCWatchdog::Callback new_callback) final {
        callback = new_callback;
    }

    void lock() {
        mtx.lock();
    }

    void setup_single_channel(ChannelIndex channel, uint16_t low, uint16_t high, awdg::ADCWatchdog::Callback &&callback) {
        ADC_AnalogWDGConfTypeDef init {
            .WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG,
            .HighThreshold = high,
            .LowThreshold = low,
            .Channel = channel,
            .ITMode = ENABLE,
            .WatchdogNumber = 0, // UNUSED
        };

        if (HAL_ADC_AnalogWDGConfig(hadc, &init) != HAL_OK) {
            bsod("ADC Watchdog misconfigured");
        }

        this->callback = std::move(callback);
    }

    void setup_all_channel(uint16_t low, uint16_t high, awdg::ADCWatchdog::Callback &&callback) {
        ADC_AnalogWDGConfTypeDef init {
            .WatchdogMode = ADC_ANALOGWATCHDOG_ALL_REG,
            .HighThreshold = high,
            .LowThreshold = low,
            .Channel = 0,
            .ITMode = ENABLE,
            .WatchdogNumber = 0, // UNUSED
        };

        if (HAL_ADC_AnalogWDGConfig(hadc, &init) != HAL_OK) {
            bsod("ADC Watchdog misconfigured");
        }

        this->callback = std::move(callback);
    }

    void irq_trigger() {
        debug_assert(callback != nullptr);
        callback(*this);
    }

    friend F4ADCWatchdog &get_watchdog_for_adc_handler(ADC_HandleTypeDef *hadc);

private:
    ADC_HandleTypeDef *hadc;
    freertos::Mutex mtx;
    awdg::ADCWatchdog::Callback callback;
};

static std::array awds = {
    F4ADCWatchdog(&hadc1),
    F4ADCWatchdog(&hadc2),
    F4ADCWatchdog(&hadc3),
};

F4ADCWatchdog &get_watchdog_for_adc_handler(ADC_HandleTypeDef *hadc) {
    for (auto &awd : awds) {
        if (awd.hadc == hadc) {
            return awd;
        }
    }
    bsod("Invalid ADC_HandleTypeDef ptr");
}
} // namespace awdg

awdg::ADCWatchdog::Handle awdg::get_single_channel_watchdog(void *adc, awdg::ADCWatchdog::ChannelIndex channel, awdg::ADCWatchdog::Callback &&callback, uint16_t low, uint16_t high) {
    auto &awd = get_watchdog_for_adc_handler(reinterpret_cast<ADC_HandleTypeDef *>(adc));
    awd.lock();
    awd.setup_single_channel(channel, low, high, std::move(callback));
    return awdg::ADCWatchdog::Handle(&awd);
}

awdg::ADCWatchdog::Handle awdg::get_multi_channel_watchdog([[maybe_unused]] void *adc, [[maybe_unused]] std::span<awdg::ADCWatchdog::ChannelIndex> channels, [[maybe_unused]] awdg::ADCWatchdog::Callback &&callback, [[maybe_unused]] uint16_t low, [[maybe_unused]] uint16_t high) {
    bsod("Not supported on F4 platform");
}

[[nodiscard]] awdg::ADCWatchdog::Handle awdg::get_all_channel_analog_watchdog(void *adc, awdg::ADCWatchdog::Callback &&callback, uint16_t low, uint16_t high) {
    auto &awd = get_watchdog_for_adc_handler(reinterpret_cast<ADC_HandleTypeDef *>(adc));
    awd.lock();
    awd.setup_all_channel(low, high, std::move(callback));
    return awdg::ADCWatchdog::Handle(&awd);
}

extern "C" void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc) {
    auto &awd = awdg::get_watchdog_for_adc_handler(hadc);
    awd.irq_trigger();
}

#else
    #error "Analog watchdog not implemented for selected ST platform"
#endif
