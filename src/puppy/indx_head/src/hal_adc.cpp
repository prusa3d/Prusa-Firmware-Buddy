#include "hal.hpp"

#include <fpm/fixed.hpp>
#include <stm32c0xx_hal.h>

#include <algorithm>
#include <limits>
#include <numbers>
#include <bsod/bsod.h>

namespace hal::adc {
alignas(uint32_t) std::array<uint16_t, std::to_underlying(Channel::_cnt)> impl::buffer {};

uint16_t get_raw(Channel channel) {
    debug_assert(channel != Channel::_cnt);
    return impl::buffer[std::to_underlying(channel)];
}

int16_t get_mcu_temp() {
    // Copied from https://github.com/STMicroelectronics/STM32CubeC0/blob/220515eaa28ef8848d304e6d52400aa2bbd84661/Projects/NUCLEO-C092RC/Examples_LL/ADC/ADC_MultiChannelSingleConversion_Init/Src/main.c#L258
    // and adjusted by values from datasheet and reference manual.
    uint16_t vref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(get_raw(Channel::vref_int), LL_ADC_RESOLUTION_12B);
    uint16_t *ts_cal1 = TEMPSENSOR_CAL1_ADDR;
    uint16_t ts_v_at_30 = __LL_ADC_CALC_DATA_TO_VOLTAGE(TEMPSENSOR_CAL_VREFANALOG, *ts_cal1, LL_ADC_RESOLUTION_12B);
    return __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(2'530, ts_v_at_30, TEMPSENSOR_CAL1_TEMP, vref, get_raw(Channel::mcu_temp), LL_ADC_RESOLUTION_12B);
}

template <int16_t from, int16_t to, int16_t step>
consteval auto generate_ntc_conversion_table(uint32_t resistance_at_25, uint32_t betta_factor, uint32_t second_resistance, uint32_t adc_max = 4095) {
    static_assert((to - from) % step == 0);
    static constexpr size_t item_count = ((to - from) / step) + 1;
    std::array<std::pair<uint16_t, int16_t>, item_count> conversion_table {};
    for (size_t i = 0; i < conversion_table.size(); ++i) {
        static constexpr double kelvin_25C = 298.15;
        const int16_t celsius = from + static_cast<int16_t>(i) * step;
        const double temp = static_cast<double>(celsius) + 273.15;
        const double adjusted_beta_at_temp = static_cast<double>(betta_factor) * ((1 / temp) - (1 / kelvin_25C));
        const double resistance_at_temp = resistance_at_25 * std::pow(std::numbers::e, adjusted_beta_at_temp);
        const double adc_at_temp = adc_max * (resistance_at_temp / (resistance_at_temp + second_resistance));
        conversion_table.at(i) = std::make_pair(static_cast<uint16_t>(adc_at_temp), celsius);
    }
    std::ranges::sort(conversion_table, std::less {}, [](const auto a) { return a.first; });
    return conversion_table;
}

constexpr auto nka103c1b1 = generate_ntc_conversion_table<-40, 200, 5>(10'000, 3'977, 4'700);

int16_t get_board_temp() {
    const auto raw_value = get_raw(Channel::board_temp);
    const auto it = std::ranges::lower_bound(nka103c1b1, raw_value, std::less<uint16_t> {}, [](const auto &val) { return val.first; });
    if (it == std::cbegin(nka103c1b1)) {
        return std::numeric_limits<int16_t>::max();
    }
    if (it == std::cend(nka103c1b1)) {
        return std::numeric_limits<int16_t>::min();
    }
    const auto prev = it - 1;
    const auto diff = static_cast<int32_t>(prev->first) - raw_value;
    const auto value_range = static_cast<int32_t>(it->first) - prev->first;
    const auto temp_range = static_cast<int32_t>(it->second) - prev->second;
    const auto temp_diff = diff * temp_range / value_range;

    return prev->second - temp_diff;
}

uint16_t get_input_voltage_mV() {
    // Calculate actual reference voltage of the chip [mV]
    const uint32_t v_ref_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(get_raw(Channel::vref_int), LL_ADC_RESOLUTION_12B);

    // Voltage at ADC pin [mV]
    const uint32_t v_adc_pin_mv = __LL_ADC_CALC_DATA_TO_VOLTAGE(v_ref_mv, get_raw(Channel::input_voltage), LL_ADC_RESOLUTION_12B);

    // The measured pin voltage is behind a resistor divider, calculate original voltage [mV]
    constexpr float r1 = 2 * 4700; // Ohm
    constexpr float r2 = 1000; // Ohm

    constexpr fpm::fixed_16_16 adj_coef { (r1 + r2) / r2 };

    return uint16_t { fpm::fixed_16_16 { v_adc_pin_mv } * adj_coef };
}

uint16_t get_heater_current_mA() {
    // Calculate actual reference voltage of the chip [mV]
    const uint32_t v_ref_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(get_raw(Channel::vref_int), LL_ADC_RESOLUTION_12B);

    // Voltage at ADC pin [mV] = current in mA (sensor gain: 1 V/A)
    const uint16_t raw_mA = __LL_ADC_CALC_DATA_TO_VOLTAGE(v_ref_mv, get_raw(Channel::heater_current), LL_ADC_RESOLUTION_12B);

    // There is a 100 mA offset in the current measurement of unknown origin
    constexpr uint16_t offset_mA = 100;

    return (raw_mA > offset_mA) ? (raw_mA - offset_mA) : 0;
}

} // namespace hal::adc
