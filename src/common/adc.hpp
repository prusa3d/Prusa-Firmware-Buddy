#pragma once

#include "inttypes.h"
#include <device/hal.h>
#include <device/board.h>
#include <device/peripherals.h>
#include "config_buddy_2209_02.h"
#include "sum_ring_buffer.hpp"
#include <device/hal.h>

/*
ADC channels, ranks, ...
Buddy board:
    ADC1:
        PC0  - Rank 1 - CH10 - therm 0(hotend temp)
        PA4  - Rank 2 - CH4  - therm 1(heatbed temp)
        PA5  - Rank 3 - CH5  - therm 2(board temp)
        PA6  - Rank 4 - CH6  - therm pinda
        PA3  - Rank 5 - CH3  - heatbed voltage
xBuddy>0.2.0 board:
    ADC1:
        PC0  - Rank 1 - CH10 - hotend temp
        PA4  - Rank 2 - CH4  - heatbed temp
        PA5  - Rank 3 - CH5  - heatbed voltage
        PA6  - Rank 4 - CH6  - heatbreak temp
        PA3  - Rank 5 - CH3  - hotend heater voltage

    ADC3:
        PF6  - Rank 1 - CH4  - mmu current
        PF10 - Rank 2 - CH8  - board temp
        PF3  - Rank 3 - CH9  - hotend heater current
        PF4  - Rank 4 - CH14 - input current(from PSU 24V)
        PF5  - Rank 5 - CH15 - ambient temp(external thermistor connector)
xlBuddy:
    ADC1:
        PA4  - Rank 1 - CH4  - Current sense dwarf splitter 5V supply
        PA2  - Rank 2 - CH5  - MUX1_Y
        PB0  - Rank 3 - CH8  - MUX1_X
    ADC3:
        PF10 - Rank 1 - CH8  - board temp
        PF6  - Rand 2 - CH4  - MUX2_Y
        PC0  - Rank 3 - CH10 - MUX2_Y

    PowerMonitorsHWIDAndTempMux_X:
        X0 - 24V_sense
        X1 - 5V_sense
        X2 - 5V_I_sense_sandwich_2xDwarf
        X3 - 5V_I_sense_xlBuddy
    PowerMonitorsHWIDAndTempMux_Y:
        Y0 - HW_ID_0
        Y1 - HW_ID_1
        Y2 - HW_ID_2
        Y3 - splitter_temp

    SideFilamnetSensorsAndTempMux_X:
        X0 - side_filament_sensor_1
        X1 - side_filament_sensor_2
        X2 - side_filament_sensor_3
        X3 - side_filament_sensor_4
    SideFilamnetSensorsAndTempMux_Y:
        Y0 - side_filament_sensor_5
        Y1 - side_filament_sensor_6
        Y2 - sandwich_temp
        Y3 - ambient_temp
 */

#if BOARD_IS_XLBUDDY
    #define ADC_MULTIPLEXER
#endif

/* Enums for all channels of all ADCs.
 * Underlying number of each channel serves as channel rank and index in data array.
 */
namespace AdcChannel {

#if (BOARD_IS_BUDDY)
enum AD1 { // ADC1 channels
    hotend_T,
    heatbed_T,
    board_T,
    pinda_T,
    heatbed_U,
    ADC1_CH_CNT
};
#elif (BOARD_IS_XBUDDY && PRINTER_IS_PRUSA_MK3_5)
enum AD1 { // ADC1 channels
    hotend_T,
    heatbed_T,
    heatbed_U,
    hotend_U,
    ADC1_CH_CNT
};

enum AD3 { // ADC3 channels
    MMU_I,
    board_T,
    hotend_I,
    board_I,
    case_T,
    ADC3_CH_CNT
};

#elif (BOARD_IS_XBUDDY)
enum AD1 { // ADC1 channels
    hotend_T,
    heatbed_T,
    heatbed_U,
    heatbreak_T,
    hotend_U,
    ADC1_CH_CNT
};

enum AD3 { // ADC3 channels
    MMU_I,
    board_T,
    hotend_I,
    board_I,
    case_T,
    ADC3_CH_CNT
};

#elif (BOARD_IS_XLBUDDY)
enum AD1 { // ADC1 channels
    dwarf_I,
    mux1_y,
    mux1_x,
    ADC1_CH_CNT
};

enum AD3 { // ADC3 channels
    board_T,
    mux2_y,
    mux2_x,
    ADC3_CH_CNT
};

enum PowerMonitorsHWIDAndTempMux { // Multiplexer channels
    board_U24,
    board_U5,
    sandwich_I5,
    xlbuddy_I5,
    hw_id_0,
    hw_id_1,
    hw_id_2,
    splitter_temp,
    POWER_MONITOR_HWID_AND_TEMP_CH_CNT
};

enum SideFilamnetSensorsAndTempMux { // Multiplexer channels
    sfs1,
    sfs2,
    sfs3,
    sfs4,
    sfs5,
    sfs6,
    sandwich_temp,
    ambient_temp,
    SFS_AND_TEMP_CH_CNT
};

#elif BOARD_IS_DWARF
enum AD1 {
    picked1,
    picked0,
    TFS,
    ntc,
    ntc_internal,
    heater_current,
    ntc2,
    dwarf_24V,
    mcu_temperature,
    ADC1_CH_CNT
};

#else
    #error "Unknown board."
#endif
}

inline constexpr uint16_t raw_adc_value_at_50_degreas_celsius = 993;

template <ADC_HandleTypeDef &adc, size_t channels>
class AdcDma {
public:
    AdcDma()
        : m_data()
        , buff_nozzle() {};
    void init() {
        HAL_ADC_Init(&adc);
#if BOARD_IS_DWARF
        HAL_ADCEx_Calibration_Start(&adc);
        HAL_ADCEx_Calibration_SetValue(&adc, HAL_ADC_GetValue(&adc));
#endif
        HAL_ADC_Start_DMA(&adc, reinterpret_cast<uint32_t *>(m_data), channels); // Start ADC in DMA mode and
    };

    void deinit() {
        HAL_ADC_Stop_DMA(&adc);
        HAL_ADC_DeInit(&adc);
    }

    void put_data_to_buffer() {
        buff_nozzle.Put(get_and_shift_channel(0));
    }

    [[nodiscard]] uint32_t get_sum_from_nozzle_buffer() {
        return buff_nozzle.GetSum();
    }

    [[nodiscard]] uint32_t get_size_from_nozzle_buffer() {
        return buff_nozzle.GetSize();
    }

    [[nodiscard]] uint16_t get_channel(uint8_t index) const {
        return m_data[index];
    }

    [[nodiscard]] uint16_t get_and_shift_channel(uint8_t index) const { // This function is need for convert 12bit to 10bit
        return m_data[index] >> 2;
    }

private:
    uint16_t m_data[channels] = { 0 };
    static constexpr size_t buff_size { 128 };
    SumRingBuffer<uint32_t, buff_size> buff_nozzle;
    static_assert(0xFFFF * buff_size <= 0xFFFFFFFF);
};

using AdcDma1 = AdcDma<hadc1, AdcChannel::ADC1_CH_CNT>;
extern AdcDma1 adcDma1;

#ifdef HAS_ADC3
using AdcDma3 = AdcDma<hadc3, AdcChannel::ADC3_CH_CNT>;
extern AdcDma3 adcDma3;
#endif

#ifdef ADC_MULTIPLEXER
    #include "hwio_pindef.h"
using namespace buddy::hw;

template <typename ADCDMA, size_t NUM_CHANNELS>
class AdcMultiplexer {
public:
    AdcMultiplexer(const ADCDMA &adcDma, const buddy::hw::OutputPin &selectA, const buddy::hw::OutputPin &selectB, const uint8_t adc_input_pin_x, const uint8_t adc_input_pin_y)
        : adcDma(adcDma)
        , m_setA(selectA)
        , m_setB(selectB)
        , adc_input_pin_x(adc_input_pin_x)
        , adc_input_pin_y(adc_input_pin_y) {
        static_assert(NUM_CHANNELS == 8, "This MUX class support only 8 inputs");
    }

    void switch_channel() {
        m_data[current_channel] = get_X();
        m_data[current_channel + 4] = get_Y();

        current_channel = current_channel >= 3 ? 0 : current_channel + 1;

        if (current_channel & 0b00000001) {
            m_setA.write(Pin::State::high);
        } else {
            m_setA.write(Pin::State::low);
        }

        if (current_channel & 0b00000010) {
            m_setB.write(Pin::State::high);
        } else {
            m_setB.write(Pin::State::low);
        }
    }

    [[nodiscard]] uint16_t get_channel(uint8_t index) const {
        return m_data[index];
    }

    [[nodiscard]] uint16_t get_and_shift_channel(uint8_t index) const { // This function is need for convert 12bit to 10bit because Marlin needs 10bit value

        return m_data[index] >> 2;
    }

private:
    const ADCDMA &adcDma;
    const buddy::hw::OutputPin &m_setA;
    const buddy::hw::OutputPin &m_setB;
    const uint8_t adc_input_pin_x;
    const uint8_t adc_input_pin_y;
    uint16_t m_data[NUM_CHANNELS];
    uint8_t current_channel = 0;
    uint16_t get_X() { return adcDma.get_channel(adc_input_pin_x); };
    uint16_t get_Y() { return adcDma.get_channel(adc_input_pin_y); };
};

extern AdcMultiplexer<AdcDma1, AdcChannel::POWER_MONITOR_HWID_AND_TEMP_CH_CNT> PowerHWIDAndTempMux;
extern AdcMultiplexer<AdcDma3, AdcChannel::SFS_AND_TEMP_CH_CNT> SFSAndTempMux;
#endif // ADC_MULTIPLEXER

namespace AdcGet {
#if (BOARD_IS_BUDDY)
inline uint16_t nozzle() { return adcDma1.get_and_shift_channel(AdcChannel::hotend_T); }
inline uint16_t bed() { return adcDma1.get_and_shift_channel(AdcChannel::heatbed_T); }
inline uint16_t boardTemp() { return adcDma1.get_and_shift_channel(AdcChannel::board_T); }
inline uint16_t pinda() { return adcDma1.get_and_shift_channel(AdcChannel::pinda_T); }
inline uint16_t bedMon() { return adcDma1.get_and_shift_channel(AdcChannel::heatbed_U); }
#endif

#if (BOARD_IS_XBUDDY)
inline uint16_t nozzle() {
    if (adcDma1.get_and_shift_channel(AdcChannel::hotend_T) > raw_adc_value_at_50_degreas_celsius) { // mean 50 degrees Celsius
        return (adcDma1.get_sum_from_nozzle_buffer() / adcDma1.get_size_from_nozzle_buffer());
    }
    return adcDma1.get_and_shift_channel(AdcChannel::hotend_T);
}

inline void sampleNozzle() {
    adcDma1.put_data_to_buffer();
}

inline uint16_t bed() { return adcDma1.get_and_shift_channel(AdcChannel::heatbed_T); }
    #if (!PRINTER_IS_PRUSA_MK3_5)
inline uint16_t heatbreakTemp() { return adcDma1.get_and_shift_channel(AdcChannel::heatbreak_T); }
    #endif
inline uint16_t boardTemp() { return adcDma3.get_and_shift_channel(AdcChannel::board_T); }
inline uint16_t heaterVoltage() { return adcDma1.get_and_shift_channel(AdcChannel::hotend_U); }

inline uint16_t inputVoltage() {
    return adcDma1.get_and_shift_channel(AdcChannel::heatbed_U);
}
inline uint16_t MMUCurrent() { return adcDma3.get_and_shift_channel(AdcChannel::MMU_I); }
inline uint16_t heaterCurrent() { return adcDma3.get_and_shift_channel(AdcChannel::hotend_I); }
inline uint16_t inputCurrent() { return adcDma3.get_and_shift_channel(AdcChannel::board_I); }
inline uint16_t ambientTemp() { return adcDma3.get_and_shift_channel(AdcChannel::case_T); }
#endif

#if BOARD_IS_XLBUDDY
inline uint16_t dwarfsCurrent() { return adcDma1.get_and_shift_channel(AdcChannel::dwarf_I); };
inline uint16_t boardTemp() { return adcDma3.get_and_shift_channel(AdcChannel::board_T); };
inline uint16_t inputVoltage24V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::board_U24); };
inline uint16_t inputVoltage5V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::board_U5); };
inline uint16_t sandwichCurrent5V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::sandwich_I5); };
inline uint16_t xlbuddyCurrent5V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::xlbuddy_I5); };
inline uint16_t hwId0() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::hw_id_0); };
inline uint16_t hwId1() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::hw_id_1); };
inline uint16_t hwId2() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::hw_id_2); };
inline uint16_t splitterTemp() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::splitter_temp); };
inline uint16_t side_filament_sensor(AdcChannel::SideFilamnetSensorsAndTempMux channel) {
    assert(channel >= AdcChannel::sfs1 && channel <= AdcChannel::sfs6);
    return SFSAndTempMux.get_channel(channel);
}
inline uint16_t sideFilamentSensor1() { return SFSAndTempMux.get_channel(AdcChannel::sfs1); };
inline uint16_t sideFilamentSensor2() { return SFSAndTempMux.get_channel(AdcChannel::sfs2); };
inline uint16_t sideFilamentSensor3() { return SFSAndTempMux.get_channel(AdcChannel::sfs3); };
inline uint16_t sideFilamentSensor4() { return SFSAndTempMux.get_channel(AdcChannel::sfs4); };
inline uint16_t sideFilamentSensor5() { return SFSAndTempMux.get_channel(AdcChannel::sfs5); };
inline uint16_t sideFilamentSensor6() { return SFSAndTempMux.get_channel(AdcChannel::sfs6); };
inline uint16_t sandwichTemp() { return SFSAndTempMux.get_and_shift_channel(AdcChannel::sandwich_temp); };
inline uint16_t ambientTemp() { return SFSAndTempMux.get_and_shift_channel(AdcChannel::ambient_temp); };
#endif

#if BOARD_IS_DWARF
inline uint16_t inputf24V() { return adcDma1.get_and_shift_channel(AdcChannel::dwarf_24V); }
    #if (BOARD_VER_EQUAL_TO(0, 4, 0))
inline uint16_t nozzle() { return adcDma1.get_and_shift_channel(AdcChannel::ntc2); }
    #else
inline uint16_t nozzle() { return adcDma1.get_and_shift_channel(AdcChannel::ntc); }
    #endif
inline uint16_t heaterCurrent() { return adcDma1.get_and_shift_channel(AdcChannel::heater_current); }
inline uint16_t picked0() { return adcDma1.get_channel(AdcChannel::picked0); }
inline uint16_t picked1() { return adcDma1.get_channel(AdcChannel::picked1); }
    #if (BOARD_VER_EQUAL_TO(0, 4, 0))
inline uint16_t heatbreakTemp() { return adcDma1.get_and_shift_channel(AdcChannel::ntc); }
    #else
inline uint16_t heatbreakTemp() { return adcDma1.get_and_shift_channel(AdcChannel::ntc2); }
    #endif
inline uint16_t boardTemp() { return adcDma1.get_and_shift_channel(AdcChannel::ntc_internal); }
inline uint16_t toolFimalentSensor() { return adcDma1.get_channel(AdcChannel::TFS); }
inline uint16_t mcuTemperature() { return adcDma1.get_channel(AdcChannel::mcu_temperature); }
#endif
}
