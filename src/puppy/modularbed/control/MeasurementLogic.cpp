#include "MeasurementCalculator.hpp"
#include "MeasurementLogic.hpp"
#include "HeatbedletInfo.hpp"
#include "PWMLogic.hpp"
#include "hal/HAL_System.hpp"
#include "hal/HAL_GPIO.hpp"
#include <cstring>
#include <cmath>
#include <iterator>

using namespace hal::ADCDriver;

#define CHANNEL_PREPARE_DELAY_MICROSECONDS       500
#define CHANNEL_MEASUREMENT_TIMEOUT_MICROSECONDS 5000 // actual measurement time is 2220 microseconds
#define CURRENT_STABILIZATION_TIME_MICROSECONDS  80000 // theoretical time constant of used analog RC filter is 10000 microseconds
uint16_t constexpr DEFAULT_MEASURED_VREF = 24150; // measured on real ModularBed board

#define PRECISE_MEASUREMENT_COUNT 10
#define DEFAULT_CURRENT_ADC_ZERO  32900

namespace modularbed::MeasurementLogic {

// Sequence of ADC channels is optimized, so MUX for heatbedlets 12,13,14 and 15 is switched
// just twice per measurement period and there is half-period delay between MUX switch and actual measurement, see HAL_ADC.cpp.
ADCChannel s_ADCSequence[] = {
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_14,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_15,
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_0,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_1,
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_2,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_3,
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_5,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_7,
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_8,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_9,
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_10,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_11,
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_12,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_13,
    ADCChannel::Current_A,
    ADCChannel::Heatbedlet_4,
    ADCChannel::Current_B,
    ADCChannel::Heatbedlet_6,
    ADCChannel::VREF,
    ADCChannel::MCUTemperature,
};

static_assert(ADC_BATCH_SIZE == std::size(s_ADCSequence), "s_ADCSequence invalid amount of elements");

uint16_t PreciselyMeasureChannelRaw(ADCChannel channel);
uint16_t FilterMeasuredValue(ADCChannel channel, uint16_t value);
float CalculateMeasuredValue(ADCChannel channel, uint16_t adcValue);

uint16_t s_CurrentADCZero_A = DEFAULT_CURRENT_ADC_ZERO;
uint16_t s_CurrentADCZero_B = DEFAULT_CURRENT_ADC_ZERO;
MovingAverageFilter s_CurrentFilter_A(CURRENT_FILTER_DATA_POINT_COUNT);
MovingAverageFilter s_CurrentFilter_B(CURRENT_FILTER_DATA_POINT_COUNT);

uint32_t s_ADCIndex = 0;
bool s_IsADCConversionRunning = false;
uint32_t s_ADCConversionStartTime = 0;

static uint32_t s_LastADCValues[static_cast<uint32_t>(ADCChannel::CHANNEL_COUNT)];
static float s_LastCalculatedValues[static_cast<uint32_t>(ADCChannel::CHANNEL_COUNT)];

static int32_t s_currentADCValues[PRECISE_MEASUREMENT_COUNT]; // batch of measured values for precise value filter

void Init() {
    memset(s_LastADCValues, 0, sizeof(s_LastADCValues));
    memset(s_LastCalculatedValues, 0, sizeof(s_LastCalculatedValues));

    s_LastADCValues[static_cast<uint32_t>(ADCChannel::VREF)] = DEFAULT_MEASURED_VREF;
}

void CalibrateCurrentChannels() {
    // method takes approximately 128 milliseconds on STM32G0@56MHz

    PWMLogic::DisablePWM();

    // Wait for analog RC filter to stabilize
    hal::System::WaitMicroseconds(CURRENT_STABILIZATION_TIME_MICROSECONDS);

    s_CurrentADCZero_A = PreciselyMeasureChannelRaw(ADCChannel::Current_A);
    s_CurrentADCZero_B = PreciselyMeasureChannelRaw(ADCChannel::Current_B);

    PWMLogic::EnablePWM();
}

void StartADCMeasurements() {
    StartConversion(ADCChannel::Heatbedlet_0);
    s_IsADCConversionRunning = true;
    s_ADCConversionStartTime = osKernelSysTick();
}

bool IterateADCMeasurements(ADCChannel *pChannel, float *value) {
    bool result = false;
    uint32_t now = osKernelSysTick();

    // process finished conversion
    if (s_IsADCConversionRunning && IsConversionFinished() == true) {
        s_IsADCConversionRunning = false;

        *pChannel = GetConvertedChannel();
        uint16_t adcValue = GetConversionResult();
        uint16_t filteredAdcValue = FilterMeasuredValue(*pChannel, adcValue);
        *value = CalculateMeasuredValue(*pChannel, filteredAdcValue);
        result = true;

        if (static_cast<uint32_t>(*pChannel) < static_cast<uint32_t>(ADCChannel::CHANNEL_COUNT)) {
            s_LastADCValues[static_cast<uint32_t>(*pChannel)] = adcValue;
            s_LastCalculatedValues[static_cast<uint32_t>(*pChannel)] = *value;
        }

        s_ADCIndex++;
        if (s_ADCIndex >= (sizeof(s_ADCSequence) / sizeof(ADCChannel))) {
            s_ADCIndex = 0;
        }

        PrepareConversion(s_ADCSequence[s_ADCIndex]);
    }

    // start new conversion
    if (((int32_t)(now - s_ADCConversionStartTime)) >= ADC_CONVERSION_PERIOD) {
        if (s_IsADCConversionRunning) {
            CancelConversion();
            s_IsADCConversionRunning = false;
        }

        StartConversion(s_ADCSequence[s_ADCIndex]);

        s_IsADCConversionRunning = true;
        s_ADCConversionStartTime = now;
    }

    return result;
}

uint16_t FilterMeasuredValue(ADCChannel channel, uint16_t value) {
    switch (channel) {
    case ADCChannel::Current_A:
        value = s_CurrentFilter_A.AddValue(value);
        return value;

    case ADCChannel::Current_B:
        value = s_CurrentFilter_B.AddValue(value);
        return value;

    default:
        return value;
    }
}

float CalculateMeasuredValue(ADCChannel channel, uint16_t adcValue) {
    int intValue;
    float fValue;
    HeatbedletInfo *pHBInfo;

    switch (channel) {
    case ADCChannel::Heatbedlet_0:
    case ADCChannel::Heatbedlet_1:
    case ADCChannel::Heatbedlet_2:
    case ADCChannel::Heatbedlet_3:
    case ADCChannel::Heatbedlet_4:
    case ADCChannel::Heatbedlet_5:
    case ADCChannel::Heatbedlet_6:
    case ADCChannel::Heatbedlet_7:
    case ADCChannel::Heatbedlet_8:
    case ADCChannel::Heatbedlet_9:
    case ADCChannel::Heatbedlet_10:
    case ADCChannel::Heatbedlet_11:
    case ADCChannel::Heatbedlet_12:
    case ADCChannel::Heatbedlet_13:
    case ADCChannel::Heatbedlet_14:
    case ADCChannel::Heatbedlet_15:
        pHBInfo = HeatbedletInfo::Get((uint32_t)channel);
        fValue = CalcThermistorResistance(channel, adcValue);
        pHBInfo->m_FilteredThermistorResistance.AddValue(fValue);
        fValue = fValue * pHBInfo->m_ThermistorCalibrationCoef;
        return CalcThermistorTemperature(fValue);
    case ADCChannel::Current_A:
        intValue = adcValue - s_CurrentADCZero_A;
        fValue = CalcElectricCurrent(intValue);
        return fValue;

    case ADCChannel::Current_B:
        intValue = adcValue - s_CurrentADCZero_B;
        fValue = CalcElectricCurrent(intValue);
        return fValue;

    case ADCChannel::VREF:
        return CalculateVREF(adcValue);

    case ADCChannel::MCUTemperature:
        return CalculateMCUTemperature(adcValue, s_LastADCValues[static_cast<uint32_t>(ADCChannel::VREF)]);

    default:
        return 0;
    }
}

float MeasureSingleChannel(ADCChannel channel) {
    PrepareConversion(channel);
    hal::System::WaitMicroseconds(CHANNEL_PREPARE_DELAY_MICROSECONDS);

    StartConversion(channel);

    uint32_t start = hal::System::GetMicroeconds();
    while (IsConversionFinished() == false && (hal::System::GetMicroeconds() - start) < CHANNEL_MEASUREMENT_TIMEOUT_MICROSECONDS) {
    }

    uint16_t adcValue = GetConversionResult();
    float fValue = CalculateMeasuredValue(channel, adcValue);
    return fValue;
}

float PreciselyMeasureChannel(ADCChannel channel) {
    if (channel == ADCChannel::Current_A || channel == ADCChannel::Current_B) {
        // Wait for analog RC filter to stabilize
        hal::System::WaitMicroseconds(CURRENT_STABILIZATION_TIME_MICROSECONDS);
    }

    uint16_t value = PreciselyMeasureChannelRaw(channel);
    float fValue = CalculateMeasuredValue(channel, value);
    return fValue;
}

uint16_t PreciselyMeasureChannelRaw(ADCChannel channel) {
    // prepare conversion
    PrepareConversion(channel);
    hal::System::WaitMicroseconds(CHANNEL_PREPARE_DELAY_MICROSECONDS);

    // measure values
    for (int i = 0; i < PRECISE_MEASUREMENT_COUNT; i++) {
        StartConversion(channel);
        uint32_t start = hal::System::GetMicroeconds();
        while (IsConversionFinished() == false && (hal::System::GetMicroeconds() - start) < CHANNEL_MEASUREMENT_TIMEOUT_MICROSECONDS) {
        }
        s_currentADCValues[i] = GetConversionResult();
    }

    // calculate average value
    uint32_t sum = 0;
    uint32_t count = 0;
    for (int i = 0; i < PRECISE_MEASUREMENT_COUNT; i++) {
        if (s_currentADCValues[i] != -1) {
            sum += s_currentADCValues[i];
            count++;
        }
    }

    // drop values which are too different
    uint32_t average = 0;
    int dropCount = count / 2;
    for (int dropI = 0; dropI < dropCount; dropI++) {
        average = (sum + (count / 2)) / count;
        uint32_t worstDiff = 0;
        uint32_t worstIndex = 0;
        // find the most different value...
        for (int i = 0; i < PRECISE_MEASUREMENT_COUNT; i++) {
            if (s_currentADCValues[i] != -1) {
                uint32_t diff = abs((int)(average - s_currentADCValues[i]));
                if (diff > worstDiff) {
                    worstDiff = diff;
                    worstIndex = i;
                }
            }
        }
        //...and drop it
        if (worstDiff != 0) {
            sum -= s_currentADCValues[worstIndex];
            count--;
            s_currentADCValues[worstIndex] = -1;
        }
    }

    average = (sum + (count / 2)) / count;
    return (uint16_t)average;
}

float GetLastMeasuredAndCalculatedValue(ADCChannel channel) {
    if (static_cast<uint32_t>(channel) < static_cast<uint32_t>(ADCChannel::CHANNEL_COUNT)) {
        return s_LastCalculatedValues[static_cast<uint32_t>(channel)];
    } else {
        return 0;
    }
}

} // namespace modularbed::MeasurementLogic
