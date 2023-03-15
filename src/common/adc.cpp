#include "adc.hpp"
#include "hwio_pindef.h"

AdcDma1 adcDma1;

#ifdef HAS_ADC3
AdcDma3 adcDma3;
#endif

using namespace buddy::hw;

#if BOARD_IS_XLBUDDY
AdcMultiplexer<AdcDma1, AdcChannel::POWER_MONITOR_HWID_AND_TEMP_CH_CNT> PowerHWIDAndTempMux(adcDma1, buddy::hw::AD1setA, buddy::hw::AD1setB, AdcChannel::mux1_x, AdcChannel::mux1_y);

AdcMultiplexer<AdcDma3, AdcChannel::SFS_AND_TEMP_CH_CNT> SFSAndTempMux(adcDma3, buddy::hw::AD2setA, buddy::hw::AD2setB, AdcChannel::mux2_x, AdcChannel::mux2_y);

#endif
