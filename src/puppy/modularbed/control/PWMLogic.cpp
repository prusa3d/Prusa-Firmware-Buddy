#include "StateLogic.hpp"
#include "HeatbedletInfo.hpp"
#include "hal/HAL_PWM.hpp"
#include "PWMLogic.hpp"
#include "MeasurementCalculator.hpp"
#include "MeasurementLogic.hpp"
#include "cmsis_os.h"
#include <cmath>
#include <array>
#include <cassert>

static constexpr std::array<std::size_t, Branch::count> HB_COUNT_BRANCH { 10, 6 };

namespace modularbed::PWMLogic {

struct HBBranchInfo {
    uint32_t HBCount;
    HeatbedletInfo **pHBList;
    float MaxCurrent;
    float RampingStartCurrent;
    float RampingEndCurrent;
    float OvercurrentThreshold;

    float LastTotalCurrent;
    float CurrentLimiterCoef;
};

static bool s_PWMEnabled = false;
static std::array<float, 2> s_ExpectedCurrent = { 0, 0 };

static HeatbedletInfo *s_SortedHBInfo_A[HB_COUNT_BRANCH[Branch::A]];
static HeatbedletInfo *s_SortedHBInfo_B[HB_COUNT_BRANCH[Branch::B]];

static HBBranchInfo s_HBBranch_A;
static HBBranchInfo s_HBBranch_B;

static void ApplyPWMValues(bool useLimiters);
static void ApplyCurrentLimiterForHB(uint32_t heatbedletIndex);
static void ApplyCurrentLimitersForBranch(HBBranchInfo *pHBBranchInfo, float measuredCurrent);
static float CalculatePWMLimiterBasedOnHBResistance(HBBranchInfo *pHBBranchInfo, float *pTotalCurrent);
static float CalculatePWMLimiterBasedOnMeasuredCurrent(HBBranchInfo *pHBBranchInfo, float *pTotalCurrent, float measuredCurrent);
static void ApplyPWMLimiterForBranch(HBBranchInfo *pHBBranchInfo, float coef);
static float CalculateTotalCurrentForBranch(HBBranchInfo *pHBBranchInfo);
static void ApplyPWMValuesForBranch(HBBranchInfo *pHBBranchInfo);
static void ApplySinglePWMPulse(HeatbedletInfo *pHBInfo, uint32_t *pNextFreePulse);
static void StoreValuesToModbusRegisters();

static int compareHBInfo(const void *a, const void *b) {
    return ((*(HeatbedletInfo **)b)->m_PWMValue - (*(HeatbedletInfo **)a)->m_PWMValue);
}

void Init() {
    s_SortedHBInfo_A[0] = HeatbedletInfo::Get(8);
    s_SortedHBInfo_A[1] = HeatbedletInfo::Get(9);
    s_SortedHBInfo_A[2] = HeatbedletInfo::Get(10);
    s_SortedHBInfo_A[3] = HeatbedletInfo::Get(11);
    s_SortedHBInfo_A[4] = HeatbedletInfo::Get(12);
    s_SortedHBInfo_A[5] = HeatbedletInfo::Get(13);
    s_SortedHBInfo_A[6] = HeatbedletInfo::Get(14);
    s_SortedHBInfo_A[7] = HeatbedletInfo::Get(15);
    s_SortedHBInfo_A[8] = HeatbedletInfo::Get(6);
    s_SortedHBInfo_A[9] = HeatbedletInfo::Get(7);

    s_SortedHBInfo_B[0] = HeatbedletInfo::Get(0);
    s_SortedHBInfo_B[1] = HeatbedletInfo::Get(1);
    s_SortedHBInfo_B[2] = HeatbedletInfo::Get(2);
    s_SortedHBInfo_B[3] = HeatbedletInfo::Get(3);
    s_SortedHBInfo_B[4] = HeatbedletInfo::Get(4);
    s_SortedHBInfo_B[5] = HeatbedletInfo::Get(5);

    s_HBBranch_A.HBCount = HB_COUNT_BRANCH[Branch::A];
    s_HBBranch_A.pHBList = s_SortedHBInfo_A;
    s_HBBranch_A.MaxCurrent = PWM_MAX_CURRENT_AMPS[Branch::A];
    s_HBBranch_A.RampingStartCurrent = PWM_MAX_CURRENT_AMPS[Branch::A] * PWM_RAMPING_START;
    s_HBBranch_A.RampingEndCurrent = PWM_MAX_CURRENT_AMPS[Branch::A] * PWM_RAMPING_END;
    s_HBBranch_A.OvercurrentThreshold = PWM_MAX_CURRENT_AMPS[Branch::A] * PWM_OVERCURRENT_THRESHOLD;

    s_HBBranch_A.LastTotalCurrent = 0;
    s_HBBranch_A.CurrentLimiterCoef = 1;

    s_HBBranch_B.HBCount = HB_COUNT_BRANCH[Branch::B];
    s_HBBranch_B.pHBList = s_SortedHBInfo_B;
    s_HBBranch_B.MaxCurrent = PWM_MAX_CURRENT_AMPS[Branch::B];
    s_HBBranch_B.RampingStartCurrent = PWM_MAX_CURRENT_AMPS[Branch::B] * PWM_RAMPING_START;
    s_HBBranch_B.RampingEndCurrent = PWM_MAX_CURRENT_AMPS[Branch::B] * PWM_RAMPING_END;
    s_HBBranch_B.OvercurrentThreshold = PWM_MAX_CURRENT_AMPS[Branch::B] * PWM_OVERCURRENT_THRESHOLD;

    s_HBBranch_B.LastTotalCurrent = 0;
    s_HBBranch_B.CurrentLimiterCoef = 1;
}

void EnablePWM() {
    s_PWMEnabled = true;
    ApplyPWMValues();
}

void DisablePWM() {
    s_PWMEnabled = false;
    ApplyPWMValues();
}

float GetExpectedCurrent(const uint8_t idx) {
    Branch::assert_idx(idx);
    return s_ExpectedCurrent[idx];
}

void ApplyPWMValues() {
    ApplyPWMValues(true);
}

void ApplyPWMValuesWithoutLimiters() {
    ApplyPWMValues(false);
}

void ApplyPWMValues(bool useLimiters) {
    // calculate effective PWM values
    if (useLimiters) {
        for (uint32_t hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
            ApplyCurrentLimiterForHB(hbIndex);
        }
        ApplyCurrentLimitersForBranch(&s_HBBranch_A, MeasurementLogic::GetLastMeasuredAndCalculatedValue(hal::ADCDriver::ADCChannel::Current_A));
        ApplyCurrentLimitersForBranch(&s_HBBranch_B, MeasurementLogic::GetLastMeasuredAndCalculatedValue(hal::ADCDriver::ADCChannel::Current_B));
    }

    s_ExpectedCurrent[Branch::A] = CalculateTotalCurrentForBranch(&s_HBBranch_A);
    s_ExpectedCurrent[Branch::B] = CalculateTotalCurrentForBranch(&s_HBBranch_B);

    // calculate pulse widths
    for (uint32_t hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
        pHBInfo->m_PWMPulseLength = ((uint32_t)(pHBInfo->m_PWMValue * PWM_PERIOD_LENGTH));
        pHBInfo->m_RoundedPWMValue = ((float)pHBInfo->m_PWMPulseLength) / PWM_PERIOD_LENGTH;
    }

    // set PWM pulses to PWM driver
    if (s_PWMEnabled) {
        ApplyPWMValuesForBranch(&s_HBBranch_A);
        ApplyPWMValuesForBranch(&s_HBBranch_B);
    }

    // switch PWM pattern in PWM driver
    hal::PWMDriver::ApplyPWMPattern();

    // publish calculated values to Modbus registers
    StoreValuesToModbusRegisters();
}

void ApplyCurrentLimiterForHB(uint32_t heatbedletIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);
    float referenceResistance = pHBInfo->m_ReferenceResistance;
    float temperature = pHBInfo->m_MeasuredTemperature;
    float maxCurrent = pHBInfo->m_MaxAllowedCurrent;
    float resistanceForMaxCurrent = CalcHBReferenceResistance(maxCurrent, temperature);

    if (resistanceForMaxCurrent > referenceResistance) {
        float maxPWMValue = referenceResistance / resistanceForMaxCurrent;
        if (pHBInfo->m_PWMValue > maxPWMValue) {
            pHBInfo->m_PWMValue = maxPWMValue;
            pHBInfo->m_IsPWMUpperSaturated = true;
        }
    }
}

void ApplyCurrentLimitersForBranch(HBBranchInfo *pHBBranchInfo, float measuredCurrent) {
    // calculate all limiters
    float totalCurrent = CalculateTotalCurrentForBranch(pHBBranchInfo);
    float coef_HB_resistance = CalculatePWMLimiterBasedOnHBResistance(pHBBranchInfo, &totalCurrent);
    float coef_measured_current = CalculatePWMLimiterBasedOnMeasuredCurrent(pHBBranchInfo, &totalCurrent, measuredCurrent);

    // apply pwm limiter
    ApplyPWMLimiterForBranch(pHBBranchInfo, coef_HB_resistance * coef_measured_current);
}

float CalculatePWMLimiterBasedOnHBResistance(HBBranchInfo *pHBBranchInfo, float *pTotalCurrent) {
    // This function calculates PWM limiter, so expected total PSU current will not be higher than maximal PSU current.
    // Limiter is based on HB resistance recalculated to measured temperature.
    // But this limiter is not precise enough, because actual and measured temperature may differ significantly,
    // when user places cold steel on hot bed. Because of this, there is also limiter based on measured current, see below.

    float coef = 1;
    if (*pTotalCurrent > pHBBranchInfo->MaxCurrent) {
        coef = pHBBranchInfo->MaxCurrent / *pTotalCurrent;
        *pTotalCurrent = pHBBranchInfo->MaxCurrent;
    }

    return coef;
}

float CalculatePWMLimiterBasedOnMeasuredCurrent(HBBranchInfo *pHBBranchInfo, float *pTotalCurrent, float measuredCurrent) {
    // This function calculates PWM limiter, so actual PSU current will never be higher than maximal allowed PSU current.
    // Limiter is based on measured PSU current. Unfortunately, there is time delay between actual PSU current and measured PSU current.
    // Because of this, limiter performs soft current ramping when current is close to maximal current.
    // There is also overcurrent protection: when measured current is higher than specified threshold, then PWM is lowered instantly and then ramped-up again.
    // This limiter is designed and tested for situatuations, when user places cold steel sheet onto bed, which is heated to high temperature.
    // Please also see configuration constants PWM_RAMPING_START, PWM_RAMPING_END, PWM_OVERCURRENT_THRESHOLD, PWM_RAMPING_SPEED

    float coef = pHBBranchInfo->CurrentLimiterCoef;

    // if expected current is higher than threshold for ramping, then use ramping
    if (*pTotalCurrent > pHBBranchInfo->RampingStartCurrent) {
        // if previous current was too small, then simulate that last current is ramping start current
        if (pHBBranchInfo->LastTotalCurrent < pHBBranchInfo->RampingStartCurrent) {
            pHBBranchInfo->LastTotalCurrent = pHBBranchInfo->RampingStartCurrent;
        }

        // adjust limiter, so current will be increased gradually using ramping
        coef *= pHBBranchInfo->LastTotalCurrent / *pTotalCurrent;
    } else {
        // expected current is lower than threshold, no ramping is necessary
        coef = 1;
    }

    pHBBranchInfo->LastTotalCurrent = *pTotalCurrent;

    // do soft current ramping
    coef += (pHBBranchInfo->RampingEndCurrent - measuredCurrent) * PWM_RAMPING_SPEED;

    // limit maximum coeffiiciet value
    if (coef > 1) {
        coef = 1;
    }

    // detect overcurrent
    if (measuredCurrent > pHBBranchInfo->OvercurrentThreshold) {
        // start ramping again
        coef = PWM_RAMPING_START;
    };

    pHBBranchInfo->CurrentLimiterCoef = coef;
    return coef;
}

void ApplyPWMLimiterForBranch(HBBranchInfo *pHBBranchInfo, float coef) {
    if (coef < 1) {
        for (uint32_t hbIndex = 0; hbIndex < pHBBranchInfo->HBCount; hbIndex++) {
            if (pHBBranchInfo->pHBList[hbIndex]->m_PWMValue > 0) {
                pHBBranchInfo->pHBList[hbIndex]->m_PWMValue *= coef;
                pHBBranchInfo->pHBList[hbIndex]->m_IsPWMUpperSaturated = true;
            }
        }
    }
}

float CalculateTotalCurrentForBranch(HBBranchInfo *pHBBranchInfo) {
    float totalCurrent = 0;

    for (uint32_t hbIndex = 0; hbIndex < pHBBranchInfo->HBCount; hbIndex++) {
        HeatbedletInfo *pHBInfo = pHBBranchInfo->pHBList[hbIndex];

        if (pHBInfo->m_PWMValue > 0) {
            float referenceResistance = pHBInfo->m_ReferenceResistance;
            float temperature = pHBInfo->m_MeasuredTemperature;
            float resistance = CalcHBResistanceAtTemperature(referenceResistance, temperature);
            float current = (((float)HEATBEDLET_VOLTAGE) / resistance) * pHBInfo->m_PWMValue;
            totalCurrent += current;
        }
    }

    return totalCurrent;
}

void ApplyPWMValuesForBranch(HBBranchInfo *pHBBranchInfo) {
    // sort HB info list, so shortest pulse will be the last
    // and it will be possible to spread empty pulse spaces
    qsort(pHBBranchInfo->pHBList, pHBBranchInfo->HBCount, sizeof(HeatbedletInfo *), compareHBInfo);

    // count used pulses and HBs
    uint32_t remainingHBCount = 0;
    uint32_t remainingPulseLengthSum = 0;
    for (uint32_t hbIndex = 0; hbIndex < pHBBranchInfo->HBCount; hbIndex++) {
        if (pHBBranchInfo->pHBList[hbIndex]->m_PWMPulseLength > 0) {
            remainingHBCount++;
            remainingPulseLengthSum += pHBBranchInfo->pHBList[hbIndex]->m_PWMPulseLength;
        }
    }

    uint32_t nextFreePulse = 0;

    // generate pulse starts and ends
    for (uint32_t hbIndex = 0; hbIndex < pHBBranchInfo->HBCount; hbIndex++) {
        HeatbedletInfo *pHBInfo = pHBBranchInfo->pHBList[hbIndex];

        if (pHBInfo->m_PWMPulseLength > 0) {
            if (remainingPulseLengthSum < PWM_PERIOD_LENGTH && nextFreePulse > 0) {
                int32_t remainingEmptyPulseLength = PWM_PERIOD_LENGTH - nextFreePulse - remainingPulseLengthSum;
                if (remainingEmptyPulseLength > 0) {
                    nextFreePulse += remainingEmptyPulseLength / (remainingHBCount + 1);
                }
            }

            ApplySinglePWMPulse(pHBInfo, &nextFreePulse);
            remainingHBCount--;
            remainingPulseLengthSum -= pHBInfo->m_PWMPulseLength;
        }
    }
}

void ApplySinglePWMPulse(HeatbedletInfo *pHBInfo, uint32_t *pNextFreePulse) {
    uint32_t pulseLength = pHBInfo->m_PWMPulseLength;

    if (pulseLength > 0) {
        uint32_t pulseStartEdge = *pNextFreePulse;
        (*pNextFreePulse) += pulseLength;
        if (*pNextFreePulse >= PWM_PERIOD_LENGTH) {
            *pNextFreePulse -= PWM_PERIOD_LENGTH;
        }

        hal::PWMDriver::AddPWMPulse(pHBInfo->m_HBIndex, pulseStartEdge, pulseLength);
    }
}

void StoreValuesToModbusRegisters() {
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);

        uint16_t modbusPWM = (uint16_t)(pHBInfo->m_RoundedPWMValue * ((float)MODBUS_PWM_STATE_SCALE) + 0.5f);
        ModbusRegisters::SetRegValue(ModbusRegisters::HBInputRegister::pwm_state, hbIndex, modbusPWM);
    }
}

hal::ADCDriver::ADCChannel GetHBCurrentMeasurementChannel(uint32_t heatbedletIndex) {
    for (unsigned int i = 0; i < HB_COUNT_BRANCH[Branch::B]; i++) {
        if (s_SortedHBInfo_B[i]->m_HBIndex == heatbedletIndex) {
            return hal::ADCDriver::ADCChannel::Current_B;
        }
    }

    return hal::ADCDriver::ADCChannel::Current_A;
}

} // namespace modularbed::PWMLogic
