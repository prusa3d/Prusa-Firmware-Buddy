#include "adc.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <utility>

#include <stm32c0xx_hal.h>
#include <bsod/bsod.h>

namespace adc {
namespace {
    constexpr auto ntcg104lh104jdts_conversion_table = std::to_array<std::pair<Raw<16>, Temperature>>({
        { Raw<16> { 0 }, 700 },
        { Raw<16> { 0x4e00 }, 125 },
        { Raw<16> { 0x5680 }, 120 },
        { Raw<16> { 0x5fc0 }, 115 },
        { Raw<16> { 0x6980 }, 110 },
        { Raw<16> { 0x73c0 }, 105 },
        { Raw<16> { 0x7e40 }, 100 },
        { Raw<16> { 0x8940 }, 95 },
        { Raw<16> { 0x9440 }, 90 },
        { Raw<16> { 0x9f40 }, 85 },
        { Raw<16> { 0xaa00 }, 80 },
        { Raw<16> { 0xb480 }, 75 },
        { Raw<16> { 0xbe40 }, 70 },
        { Raw<16> { 0xc780 }, 65 },
        { Raw<16> { 0xd000 }, 60 },
        { Raw<16> { 0xd780 }, 55 },
        { Raw<16> { 0xde40 }, 50 },
        { Raw<16> { 0xe440 }, 45 },
        { Raw<16> { 0xe940 }, 40 },
        { Raw<16> { 0xed80 }, 35 },
        { Raw<16> { 0xf140 }, 30 },
        { Raw<16> { 0xf440 }, 25 },
        { Raw<16> { 0xf6c0 }, 20 },
        { Raw<16> { 0xf8c0 }, 15 },
        { Raw<16> { 0xfa40 }, 10 },
        { Raw<16> { 0xfbc0 }, 5 },
        { Raw<16> { 0xfcc0 }, 0 },
        { Raw<16> { 0xfd80 }, -5 },
        { Raw<16> { 0xfe00 }, -10 },
        { Raw<16> { 0xfe80 }, -15 },
        { Raw<16> { 0xfec0 }, -20 },
        { Raw<16> { 0xff00 }, -25 },
        { Raw<16> { 0xff40 }, -30 },
    });

    constexpr Temperature calc_board_temp(Raw<16> raw_value) {
        const auto it = std::ranges::lower_bound(ntcg104lh104jdts_conversion_table, raw_value, std::less<Raw<16>> {}, [](const auto &val) { return val.first; });
        if (it == std::end(ntcg104lh104jdts_conversion_table)) {
            return std::numeric_limits<int16_t>::min();
        }
        const auto prev = it - 1;
        const auto diff = prev->first.to_raw() - raw_value.to_raw();
        const auto value_range = it->first.to_raw() - prev->first.to_raw();
        const auto temp_range = it->second - prev->second;
        const auto temp_diff = diff * temp_range / value_range;

        return prev->second - temp_diff;
    }

    // Test range of easily computable temperatures
    static_assert(calc_board_temp(Raw<16> { 0xf440 }) == 25);
    static_assert(calc_board_temp(Raw<16> { 0xf4c0 }) == 24);
    static_assert(calc_board_temp(Raw<16> { 0xf540 }) == 23);
    static_assert(calc_board_temp(Raw<16> { 0xf5c0 }) == 22);
    static_assert(calc_board_temp(Raw<16> { 0xf640 }) == 21);
    static_assert(calc_board_temp(Raw<16> { 0xf6c0 }) == 20);

    Temperature calc_mcu_temp(Raw<12> raw_value, Raw<12> vref_raw) {
        // Copied from https://github.com/STMicroelectronics/STM32CubeC0/blob/220515eaa28ef8848d304e6d52400aa2bbd84661/Projects/NUCLEO-C092RC/Examples_LL/ADC/ADC_MultiChannelSingleConversion_Init/Src/main.c#L258
        // and adjusted by values from datasheet and reference manual.
        uint16_t vref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(vref_raw.to_raw(), LL_ADC_RESOLUTION_12B);
        uint16_t *ts_cal1 = TEMPSENSOR_CAL1_ADDR;
        uint16_t ts_v_at_30 = __LL_ADC_CALC_DATA_TO_VOLTAGE(TEMPSENSOR_CAL_VREFANALOG, *ts_cal1, LL_ADC_RESOLUTION_12B);
        return __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(2'530, ts_v_at_30, TEMPSENSOR_CAL1_TEMP, vref, raw_value.to_raw(), LL_ADC_RESOLUTION_12B);
    }
} // namespace

alignas(uint32_t) std::array<Raw<16>, std::to_underlying(Channel::_cnt)> impl::buffer;

Raw<16> impl::get_raw(Channel channel) {
    debug_assert(std::to_underlying(channel) < impl::buffer.size());
    return impl::buffer[std::to_underlying(channel)];
}

Temperature get_mcu_temperature() {
    const auto raw_temp = impl::get_raw(Channel::mcu_temp);
    const auto vref_raw = impl::get_raw(Channel::vref_int);
    return calc_mcu_temp(raw_temp.convert_precision<12>(), vref_raw.convert_precision<12>());
}

Temperature get_board_temperature() {
    const auto raw_temp = impl::get_raw(Channel::board_temp);
    return calc_board_temp(raw_temp);
}

} // namespace adc
