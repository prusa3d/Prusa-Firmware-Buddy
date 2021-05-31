//config_a3ides2209.h - configuration file for 2209 variant (rev02)
#pragma once
#include <stdint.h>

#define PRUSA_MARLIN_API

//--------------------------------------
//DBG - debug/trace configuration
#define DBG_RTOS // use FreeRTOS (semaphore and osDelay instead of HAL_Delay)
#ifdef _DEBUG
    #define DBG_SWO // trace to swo port
    //#define DBG_UART     6 // trace to uart6 port
    //#define DBG_CDC        // trace to cdc port
    #define DBG_LEVEL 1 // debug level (0..3)
#else
//#define DBG_SWO        // trace to swo port
#endif //_DEBUG

//--------------------------------------
//WDT - watchdog timers (IWDG, WWDG)
#ifndef _DEBUG
    #define WDT_IWDG_ENABLED
//#define WDT_WWDG_ENABLED
#endif //_DEBUG

//show filament sensor status in header
//#define DEBUG_FSENSOR_IN_HEADER

//--------------------------------------
//ADC configuration
//channels:
// log pin  phy pri  function
// 0   PA3  3   1    HW_IDENTIFY
// 1   PA4  4   6    THERM1 (bed)
// 2   PA5  5   1    THERM2
// 3   PA6  6   1    THERM_PINDA
// 4   PC0  10  9    THERM0 (nozzle)
//--------------------------------------
//  bit fedc ba98 7654 3210
// mask 0000 0100 0111 1000 == 0x0478
static const uint16_t ADC_CHAN_MSK = 0x0478; //used physical AD channels bit mask (3,4,5,6,10)
enum {
    ADC_CHAN_CNT = 5, //number of used channels
};
static const uint8_t ADC_OVRSAMPL = 4; //oversampling multiplier (common for all channels)
static const uint8_t ADC_SEQ_LEN = 18; //sampling sequence length
#define ADC_SEQ2IDX adc_seq2idx        //callback function (convert seq to channel index)
#define ADC_READY   adc_ready          //callback function (value for any channel is ready)
static const uint16_t ADC_VREF = 5010; //reference voltage [mV]
//simulated values
enum {
    ADC_SIM_VAL0 = 512 * 4, //HW_IDENTIFY
    ADC_SIM_VAL1 = 966 * 4, //THERM1 (bed)     means 30C
    ADC_SIM_VAL2 = 512 * 4, //THERM2
    ADC_SIM_VAL3 = 512 * 4, //THERM_PINDA
    ADC_SIM_VAL4 = 977 * 4, //THERM0 (nozzle)  means 25C
};
//
//old seq for three channels (len = 12):
//012345678901
//220220221220
//
//new seq for five channels (len = 18):
//012345678901234567
//414140414142414143

//--------------------------------------
//FANCTL - new software pwm fan control with rpm measurement and closed loop
#define NEW_FANCTL
#ifdef NEW_FANCTL

//FANCTL0 - printing fan
//static const uint8_t FANCTL0_PWM_MIN = 15;
static const uint8_t FANCTL0_PWM_MIN = 10;
static const uint8_t FANCTL0_PWM_MAX = 50;
static const uint16_t FANCTL0_RPM_MIN = 500;
static const uint16_t FANCTL0_RPM_MAX = 5000;
static const uint8_t FANCTL0_PWM_THR = 20;
    #define FANCTL0_TRACE
//FANCTL1 - heatbreak fan
//static const uint8_t FANCTL1_PWM_MIN = 12;
static const uint8_t FANCTL1_PWM_MIN = 0;
static const uint8_t FANCTL1_PWM_MAX = 50;
static const uint16_t FANCTL1_RPM_MIN = 1000;
static const uint16_t FANCTL1_RPM_MAX = 8000;
static const uint8_t FANCTL1_PWM_THR = 20;
    #define FANCTL1_TRACE

#endif //NEW_FANCTL

//Simulator configuration
//#define SIM_HEATER
//#define SIM_MOTION

#ifdef SIM_HEATER
    #define ADC_SIM_MSK           0x0012 //simulated logical AD channels bit mask (1,4)
    #define SIM_HEATER_NOZZLE_ADC 4      //
    #define SIM_HEATER_BED_ADC    1      //
#endif                                   //SIM_HEATER

#ifdef SIM_MOTION
//#define SIM_MOTION_TRACE_X
//#define SIM_MOTION_TRACE_Y
//#define SIM_MOTION_TRACE_Z
#endif //SIM_MOTION

//new pause settings
static const uint8_t PAUSE_NOZZLE_TIMEOUT = 45; // nozzle "sleep" after 45s inside paused state
