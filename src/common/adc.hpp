#pragma once

#include "inttypes.h"
#include <device/hal.h>
#include <device/board.h>
#include <device/peripherals.h>
#include <stdint.h>
#include "printers.h"
#include "MarlinPin.h"
#include "sum_ring_buffer.hpp"
#include <device/hal.h>
#include <limits>

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
        PC0  - Rank 3 - CH10 - MUX2_X

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

#if BOARD_IS_XLBUDDY()
    #define ADC_MULTIPLEXER
#endif

/* Enums for all channels of all ADCs.
 * Underlying number of each channel serves as channel rank and index in data array.
 */
namespace AdcChannel {

#if (BOARD_IS_BUDDY())
enum AD1 { // ADC1 channels
    hotend_T,
    heatbed_T,
    board_T,
    pinda_T,
    heatbed_U,
    mcu_temperature,
    vref,
    ADC1_CH_CNT
};
#elif (BOARD_IS_XBUDDY() && PRINTER_IS_PRUSA_MK3_5())
enum AD1 { // ADC1 channels
    hotend_T,
    heatbed_T,
    heatbed_U,
    hotend_U,
    vref,
    mcu_temperature,
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

#elif (BOARD_IS_XBUDDY())
enum AD1 { // ADC1 channels
    hotend_T,
    heatbed_T,
    heatbed_U,
    heatbreak_T,
    hotend_U,
    vref,
    mcu_temperature,
    ADC1_CH_CNT
};

enum AD3 { // ADC3 channels
    MMU_I,
    board_T,
    hotend_I,
    board_I,
    #if PRINTER_IS_PRUSA_iX()
    case_T,
    #elif PRINTER_IS_PRUSA_COREONE()
    door_sensor,
    #endif
    ADC3_CH_CNT
};

#elif (BOARD_IS_XLBUDDY())
enum AD1 { // ADC1 channels
    dwarf_I,
    mux1_y,
    mux1_x,
    vref,
    mcu_temperature,
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

#elif BOARD_IS_DWARF()
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
    vref,
    ADC1_CH_CNT
};

#else
    #error "Unknown board."
#endif
} // namespace AdcChannel

inline constexpr uint16_t raw_adc_value_at_50_degreas_celsius = 993;

template <ADC_HandleTypeDef &adc, size_t channels>
class AdcDma {
public:
    static constexpr ADC_HandleTypeDef &handle = adc;
    static constexpr uint16_t reset_value = UINT16_MAX;

    // Sampling resolution
    static constexpr uint16_t sample_bits = 12;
    static constexpr uint16_t sample_max = (1 << sample_bits) - 1;

    // Shift bits required to reduce from the full 12bit resolution to 10bit as expected by Marlin
    static constexpr uint16_t shift_bits = 2;

    AdcDma()
        : m_data() {}

    void init() {
        // Ensure ADC is initialized to 12bit resolution for shifts to make sense
        assert(adc.Init.Resolution == ADC_RESOLUTION_12B);

        HAL_ADC_Init(&adc);
#if BOARD_IS_DWARF()
        HAL_ADCEx_Calibration_Start(&adc);
        HAL_ADCEx_Calibration_SetValue(&adc, HAL_ADC_GetValue(&adc));
#endif
        HAL_ADC_Start_DMA(&adc, reinterpret_cast<uint32_t *>(m_data), channels); // Start ADC in DMA mode
    }

    void deinit() {
        HAL_ADC_Stop_DMA(&adc);
        HAL_ADC_DeInit(&adc);
    }

#ifdef ADC_MULTIPLEXER
    void restart() {
        // stop and reset ADC to discard any pending conversion
        ADC_HandleTypeDef &adc_handle = handle;
        __HAL_ADC_DISABLE(&adc_handle);
        ADC_STATE_CLR_SET(adc_handle.State,
            HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
            HAL_ADC_STATE_READY);

        // NOTE: when re-enabling the ADC normally a settling time is required, which we skip here
        //   as we're inside an ISR. On the STM32G0 calibration might be required again. We
        //   re-enable it ASAP before sampling is done, but it can still result in extra noise. An
        //   extra wait iteration might be required if this sample requires higher precision.
        __HAL_ADC_ENABLE(&handle);

        // reset and restart DMA
        // TODO: this is specific to STM32F4
        DMA_HandleTypeDef &dma_handle = *adc_handle.DMA_Handle;
        __HAL_DMA_CLEAR_FLAG(&dma_handle, (DMA_FLAG_HTIF0_4 | DMA_FLAG_TCIF0_4));
        dma_handle.State = HAL_DMA_STATE_READY;
        __HAL_DMA_ENABLE(&dma_handle);

        // kickoff the first conversion
        handle.Instance->CR2 |= ADC_CR2_SWSTART;
    }
#endif // ADC_MULTIPLEXER

    [[nodiscard]] uint16_t get_channel(uint8_t index) const {
        return m_data[index];
    }

    void set_channel(uint8_t index, uint16_t value) {
        m_data[index] = value;
    }

    // Downscale from ADC full resolution as required by Marlin
    [[nodiscard]] uint16_t get_and_shift_channel(uint8_t index) const {
        return get_channel(index) >> shift_bits;
    }

private:
    uint16_t m_data[channels] = { reset_value };
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

/**
 * @brief ADC 2x2 Multiplexer with DMA support
 *
 * get_channel()/get_channel_and_shift() return current values (as determined by the ADCDMA sampling
 * interval) as long as the requested channel is active. If the channel is not active, the most
 * recent updated value (from the latest sampled value before the channel switch) is returned
 * otherwise.
 *
 * Notes on DMA and channel switching:
 *
 * The channel switching mechanism takes control of DMA_IRQn, which should be set to the IRQ of the
 * selected DMA stream. It is used exclusively for synchronization and is expected to be disabled on
 * start. When a channel switch is requested the IRQ is temporarily enabled and the switch happens
 * at some point in the future.
 *
 * AdcDma keeps the ADC in continuous scanning mode, triggering a DMA request for each EOC
 * interrupt. The EOC interrupt can't be used to perform channel switching since a DMA request can
 * still be in-flight when handled, potentially overwriting any value in the destination buffer with
 * a sample from the previous channel. For this reason the muxer temporarily shuts down the DMA
 * stream (keeping the ADC in SCAN mode to ensure at least one more pending interrupt is generated)
 * and handles the switch only when the DMA stream has stopped.
 *
 * By the time the channel switch can happen though the ADC can still be anywhere in the scanning
 * sequence, and at least on the STM32F4 there's no way to read the current index besides manually
 * keeping track of the EOC interrupt manually. The index cannot be recovered from the DMA offset
 * either, since by the time the DMA interrupt is delivered the ADC has already advanced. In the
 * transfer-complete IRQ we thus restart the ADC with the intent of resetting the sampling sequence
 * (without changing other ADC parameters as the HAL would normally do) and restart both the ADC and
 * DMA from a zero offset. This ensures that even if the DMA stream has stopped with an error
 * condition we don't need to pay attention to the last transferred index.
 *
 * Once the channel switch is handled the DMA IRQn is no longer needed, so it is again disabled
 * until a new switch is explicitly requested to save on resources.
 */
template <typename ADCDMA, IRQn_Type DMA_IRQn, size_t NUM_CHANNELS>
class AdcMultiplexer {
public:
    static constexpr uint16_t reset_value = ADCDMA::reset_value;
    static constexpr uint8_t channel_X_off = 0; // index offset for channel X
    static constexpr uint8_t channel_Y_off = 4; // index offset for channel Y

    AdcMultiplexer(ADCDMA &adcDma,
        const buddy::hw::OutputPin &selectA, const buddy::hw::OutputPin &selectB,
        const uint8_t adc_input_pin_x, const uint8_t adc_input_pin_y)
        : adcDma(adcDma)
        , m_setA(selectA)
        , m_setB(selectB)
        , adc_input_pin_x(adc_input_pin_x)
        , adc_input_pin_y(adc_input_pin_y) {
        static_assert(NUM_CHANNELS == 8, "This MUX class support only 8 inputs");
    }

    void conv_isr() {
        if ((adcDma.handle.DMA_Handle->Instance->CR & DMA_SxCR_EN) != RESET) {
            return; // not disabled yet, we'll get another interrupt
        }

        // update current values one last time
        update_X();
        update_Y();

        // change channel
        current_channel = (current_channel + 1) % 4;
        m_setA.write((Pin::State)(current_channel & 0b01));
        m_setB.write((Pin::State)(current_channel & 0b10));

        // preset previously sampled values in the DMA buffer
        // until they are overwritten by the new sample/transfer
        adcDma.set_channel(adc_input_pin_x, m_data[current_channel + channel_X_off]);
        adcDma.set_channel(adc_input_pin_y, m_data[current_channel + channel_Y_off]);

        // disable DMA updates until next switch request
        HAL_NVIC_DisableIRQ(DMA_IRQn);

        // reset and restart ADC/DMA on the new channel
        adcDma.restart();
    }

    void switch_channel() {
        // prepare to switch channel
        HAL_NVIC_EnableIRQ(DMA_IRQn);
        __HAL_DMA_DISABLE(adcDma.handle.DMA_Handle);
    }

    void read_all_channels() {
        for (size_t i = 0; i < NUM_CHANNELS / 2; ++i) {
            // Switch channel and wait for it to be updated
            switch_channel();
            osDelay(1);
        }
    }

    [[nodiscard]] const ADC_HandleTypeDef &adc_handle() const {
        return adcDma.handle;
    }

    [[nodiscard]] uint16_t get_channel(uint8_t index) {
        // update current values
        if (index == (current_channel + channel_X_off)) {
            update_X();
        } else if (index == (current_channel + channel_Y_off)) {
            update_Y();
        }

        return m_data[index];
    }

    // Downscale from ADC full resolution as required by Marlin
    [[nodiscard]] uint16_t get_and_shift_channel(uint8_t index) {
        return get_channel(index) >> ADCDMA::shift_bits;
    }

private:
    ADCDMA &adcDma;
    const buddy::hw::OutputPin &m_setA;
    const buddy::hw::OutputPin &m_setB;
    const uint8_t adc_input_pin_x;
    const uint8_t adc_input_pin_y;
    uint16_t m_data[NUM_CHANNELS];
    uint8_t current_channel = 0;

    void update_X() {
        m_data[current_channel + channel_X_off] = adcDma.get_channel(adc_input_pin_x);
    }

    void update_Y() {
        m_data[current_channel + channel_Y_off] = adcDma.get_channel(adc_input_pin_y);
    }
};

extern AdcMultiplexer<AdcDma1, DMA2_Stream4_IRQn, AdcChannel::POWER_MONITOR_HWID_AND_TEMP_CH_CNT> PowerHWIDAndTempMux;
extern AdcMultiplexer<AdcDma3, DMA2_Stream0_IRQn, AdcChannel::SFS_AND_TEMP_CH_CNT> SFSAndTempMux;
#endif // ADC_MULTIPLEXER

namespace AdcGet {
static constexpr uint16_t undefined_value = AdcDma1::reset_value;

#if (BOARD_IS_BUDDY())
inline uint16_t nozzle() { return adcDma1.get_and_shift_channel(AdcChannel::hotend_T); }
inline uint16_t bed() { return adcDma1.get_and_shift_channel(AdcChannel::heatbed_T); }
inline uint16_t boardTemp() { return adcDma1.get_and_shift_channel(AdcChannel::board_T); }
inline uint16_t pinda() { return adcDma1.get_and_shift_channel(AdcChannel::pinda_T); }
inline uint16_t bedMon() { return adcDma1.get_and_shift_channel(AdcChannel::heatbed_U); }
inline uint16_t vref() { return adcDma1.get_channel(AdcChannel::vref); } ///< Internal reference necessary for mcu_temperature
inline uint16_t mcuTemperature() { return adcDma1.get_channel(AdcChannel::mcu_temperature); } ///< Raw sensor, use getMCUTemp() instead
#endif

#if (BOARD_IS_XBUDDY())
static constexpr size_t nozzle_buff_size { 128 };
extern SumRingBuffer<uint32_t, nozzle_buff_size> nozzle_ring_buff;
static_assert((adcDma1.sample_max * nozzle_buff_size) <= std::numeric_limits<decltype(nozzle_ring_buff)::sum_type>::max(),
    "Sum buffer type can overflow");

inline uint16_t nozzle() {
    auto raw_temp = adcDma1.get_and_shift_channel(AdcChannel::hotend_T);

    // increase oversampling for values lower than 50 degrees Celsius to reduce noise
    if (raw_temp > raw_adc_value_at_50_degreas_celsius) {
        // handle an empty buffer
        if (nozzle_ring_buff.GetSize() == 0) {
            return adcDma1.reset_value;
        }

        // decimate to match the behavior of get_and_shift_channel()
        auto raw_temp_avg = nozzle_ring_buff.GetSum() / nozzle_ring_buff.GetSize();
        return raw_temp_avg >> adcDma1.shift_bits;
    }

    return raw_temp;
}

inline void sampleNozzle() {
    // the ring buffer is kept at full resolution
    auto raw_temp = adcDma1.get_channel(AdcChannel::hotend_T);
    if (raw_temp == adcDma1.reset_value) {
        nozzle_ring_buff.PopLast();
    } else {
        nozzle_ring_buff.Put(raw_temp);
    }
}

inline uint16_t bed() { return adcDma1.get_and_shift_channel(AdcChannel::heatbed_T); }
    #if (!PRINTER_IS_PRUSA_MK3_5())
inline uint16_t heatbreakTemp() { return adcDma1.get_and_shift_channel(AdcChannel::heatbreak_T); }
    #endif
inline uint16_t boardTemp() { return adcDma3.get_and_shift_channel(AdcChannel::board_T); }
inline uint16_t heaterVoltage() { return adcDma1.get_and_shift_channel(AdcChannel::hotend_U); }

inline uint16_t inputVoltage() {
    return adcDma1.get_and_shift_channel(AdcChannel::heatbed_U);
}

    #if PRINTER_IS_PRUSA_iX()
inline uint16_t psu_temp() { return adcDma1.get_and_shift_channel(AdcChannel::heatbed_T); }
inline uint16_t ambient_temp() { return adcDma3.get_and_shift_channel(AdcChannel::case_T); }
    #elif PRINTER_IS_PRUSA_COREONE()
inline uint16_t door_sensor() { return adcDma3.get_channel(AdcChannel::door_sensor); }
    #endif

inline uint16_t MMUCurrent() { return adcDma3.get_and_shift_channel(AdcChannel::MMU_I); }
inline uint16_t heaterCurrent() { return adcDma3.get_and_shift_channel(AdcChannel::hotend_I); }
inline uint16_t inputCurrent() { return adcDma3.get_and_shift_channel(AdcChannel::board_I); }
inline uint16_t vref() { return adcDma1.get_channel(AdcChannel::vref); } ///< Internal reference necessary for mcu_temperature
inline uint16_t mcuTemperature() { return adcDma1.get_channel(AdcChannel::mcu_temperature); } ///< Raw sensor, use getMCUTemp() instead
#endif

#if BOARD_IS_XLBUDDY()
inline uint16_t dwarfsCurrent() { return adcDma1.get_and_shift_channel(AdcChannel::dwarf_I); };
inline uint16_t boardTemp() { return adcDma3.get_and_shift_channel(AdcChannel::board_T); };
inline uint16_t inputVoltage24V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::board_U24); };
inline uint16_t inputVoltage5V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::board_U5); };
inline uint16_t sandwichCurrent5V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::sandwich_I5); };
inline uint16_t xlbuddyCurrent5V() { return PowerHWIDAndTempMux.get_and_shift_channel(AdcChannel::xlbuddy_I5); };
inline uint16_t hwId0() { return PowerHWIDAndTempMux.get_channel(AdcChannel::hw_id_0); };
inline uint16_t hwId1() { return PowerHWIDAndTempMux.get_channel(AdcChannel::hw_id_1); };
inline uint16_t hwId2() { return PowerHWIDAndTempMux.get_channel(AdcChannel::hw_id_2); };
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
inline uint16_t vref() { return adcDma1.get_channel(AdcChannel::vref); } ///< Internal reference necessary for mcu_temperature
inline uint16_t mcuTemperature() { return adcDma1.get_channel(AdcChannel::mcu_temperature); } ///< Raw sensor, use getMCUTemp() instead
#endif

#if BOARD_IS_DWARF()
inline uint16_t inputf24V() { return adcDma1.get_and_shift_channel(AdcChannel::dwarf_24V); }
inline uint16_t nozzle() { return adcDma1.get_and_shift_channel(AdcChannel::ntc); }
inline uint16_t heaterCurrent() { return adcDma1.get_and_shift_channel(AdcChannel::heater_current); }
inline uint16_t picked0() { return adcDma1.get_channel(AdcChannel::picked0); }
inline uint16_t picked1() { return adcDma1.get_channel(AdcChannel::picked1); }
inline uint16_t heatbreakTemp() { return adcDma1.get_and_shift_channel(AdcChannel::ntc2); }
inline uint16_t boardTemp() { return adcDma1.get_and_shift_channel(AdcChannel::ntc_internal); }
inline uint16_t toolFimalentSensor() { return adcDma1.get_channel(AdcChannel::TFS); }
inline uint16_t mcuTemperature() { return adcDma1.get_channel(AdcChannel::mcu_temperature); } ///< Raw sensor, use getMCUTemp() instead
inline uint16_t vref() { return adcDma1.get_channel(AdcChannel::vref); } ///< Internal reference necessary for mcu_temperature
#endif

int32_t getMCUTemp(); ///< Internal MCU temperature sensor [degrees C]

} // namespace AdcGet
