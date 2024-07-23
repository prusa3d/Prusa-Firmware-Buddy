#include "adc.hpp"
#include "hwio_pindef.h"

AdcDma1 adcDma1;

#ifdef HAS_ADC3
AdcDma3 adcDma3;
#endif

using namespace buddy::hw;

#if BOARD_IS_XBUDDY()
namespace AdcGet {
SumRingBuffer<uint32_t, nozzle_buff_size> nozzle_ring_buff;
}
#endif

#if BOARD_IS_XLBUDDY()
AdcMultiplexer<AdcDma1, DMA2_Stream4_IRQn, AdcChannel::POWER_MONITOR_HWID_AND_TEMP_CH_CNT>
    PowerHWIDAndTempMux(adcDma1,
        buddy::hw::AD1setA, buddy::hw::AD1setB,
        AdcChannel::mux1_x, AdcChannel::mux1_y);

AdcMultiplexer<AdcDma3, DMA2_Stream0_IRQn, AdcChannel::SFS_AND_TEMP_CH_CNT>
    SFSAndTempMux(adcDma3,
        buddy::hw::AD2setA, buddy::hw::AD2setB,
        AdcChannel::mux2_x, AdcChannel::mux2_y);

// NOTE: This ISR is currently only enabled during AdcMultiplexer channel switching,
//   and kept disabled most of the time. See AdcMultiplexer::switch_channel.
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc == &PowerHWIDAndTempMux.adc_handle()) {
        PowerHWIDAndTempMux.conv_isr();
    } else if (hadc == &SFSAndTempMux.adc_handle()) {
        SFSAndTempMux.conv_isr();
    }
}

#endif

int32_t AdcGet::getMCUTemp() {
    // there is division somewhere in formula below, so avoid division by zero in case vref is not measured properly (for example in simulator)
    if (AdcGet::vref() == 0) {
        return 0;
    }
    // Use LL calculation with factory calibration values
    ///@note ADC bit resolution shouldn't matter as it cancels out, but we use 12b on all platforms.
    return __LL_ADC_CALC_TEMPERATURE(__LL_ADC_CALC_VREFANALOG_VOLTAGE(AdcGet::vref(), LL_ADC_RESOLUTION_12B), AdcGet::mcuTemperature(), LL_ADC_RESOLUTION_12B);
}
