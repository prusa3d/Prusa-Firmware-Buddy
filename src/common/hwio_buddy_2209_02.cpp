/**
 * @file
 * @brief hardware input output abstraction for Buddy board
 */

#include <inttypes.h>

#include "hwio.h"
#include "config.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "adc.h"
#include "sim_nozzle.h"
#include "sim_bed.h"
#include "Arduino.h"
#include "timer_defaults.h"
#include "hwio_pindef.h"
#include "bsod.h"
#include "main.h"
#include "fanctl.h"
#include "MarlinPin.hpp"

/**
 * @brief hwio Marlin wrapper errors
 */
enum {
    HWIO_ERR_UNINI_DIG_RD = 0x01, //!< uninitialized digital read
    HWIO_ERR_UNINI_DIG_WR,        //!< uninitialized digital write
    HWIO_ERR_UNINI_ANA_RD,        //!< uninitialized analog read
    HWIO_ERR_UNINI_ANA_WR,        //!< uninitialized analog write
    HWIO_ERR_UNDEF_DIG_RD,        //!< undefined pin digital read
    HWIO_ERR_UNDEF_DIG_WR,        //!< undefined pin digital write
    HWIO_ERR_UNDEF_ANA_RD,        //!< undefined pin analog write
    HWIO_ERR_UNDEF_ANA_WR,        //!< undefined pin analog write
};

/**
 * @brief analog output pins
 */
const uint32_t _dac_pin32[] = {};
// a3ides analog output maximum values
const int _dac_max[] = { 0 };
static const size_t _DAC_CNT = sizeof(_dac_pin32) / sizeof(uint32_t);

enum {
    _FAN_ID_MIN = HWIO_PWM_FAN1,
    _FAN_ID_MAX = HWIO_PWM_FAN,
};
static const int _FAN_CNT = _FAN_ID_MAX - _FAN_ID_MIN + 1;

enum {
    _HEATER_ID_MIN = HWIO_PWM_HEATER_BED,
    _HEATER_ID_MAX = HWIO_PWM_HEATER_0,
};
static const int _HEATER_CNT = _HEATER_ID_MAX - _HEATER_ID_MIN + 1;

//this value is compared to new value (to avoid rounding errors)
int _tim1_period_us = GEN_PERIOD_US(TIM1_default_Prescaler, TIM1_default_Period);
int _tim3_period_us = GEN_PERIOD_US(TIM3_default_Prescaler, TIM3_default_Period);

// a3ides pwm output pins
const uint32_t _pwm_pin32[] = {
    MARLIN_PIN(HEAT0),
    MARLIN_PIN(BED_HEAT),
    MARLIN_PIN(FAN1),
    MARLIN_PIN(FAN)
};

const uint32_t _pwm_chan[] = {
    TIM_CHANNEL_3, //_PWM_HEATER_BED
    TIM_CHANNEL_4, //_PWM_HEATER_0
    TIM_CHANNEL_1, //_PWM_FAN1
    TIM_CHANNEL_2, //_PWM_FAN
};

TIM_HandleTypeDef *_pwm_p_htim[] = {
    &htim3, //_PWM_HEATER_BED
    &htim3, //_PWM_HEATER_0
    &htim1, //_PWM_FAN1
    &htim1, //_PWM_FAN
};

int *const _pwm_period_us[] = {
    &_tim3_period_us, //_PWM_HEATER_BED
    &_tim3_period_us, //_PWM_HEATER_0
    &_tim1_period_us, //_PWM_FAN1
    &_tim1_period_us, //_PWM_FAN
};

// a3ides pwm output maximum values
const int _pwm_max[] = { TIM3_default_Period, TIM3_default_Period, TIM1_default_Period, TIM1_default_Period }; //{42000, 42000, 42000, 42000};
enum {
    _PWM_CNT = (sizeof(_pwm_pin32) / sizeof(uint32_t))
};

const TIM_OC_InitTypeDef sConfigOC_default = {
    TIM_OCMODE_PWM1,       //OCMode
    0,                     //Pulse
    TIM_OCPOLARITY_HIGH,   //OCPolarity
    TIM_OCNPOLARITY_HIGH,  //OCNPolarity
    TIM_OCFAST_DISABLE,    //OCFastMode
    TIM_OCIDLESTATE_RESET, //OCIdleState
    TIM_OCNIDLESTATE_RESET //OCNIdleState
};

// a3ides pwm output maximum values  as arduino analogWrite
const int _pwm_analogWrite_max[_PWM_CNT] = { 0xff, 0xff, 0xff, 0xff };
// a3ides fan output values  as arduino analogWrite
int _pwm_analogWrite_val[_PWM_CNT] = { 0, 0, 0, 0 };

// a3ides fan output maximum values as arduino analogWrite
const int *_fan_max = &_pwm_analogWrite_max[_FAN_ID_MIN];
// a3ides fan output values as arduino analogWrite
int *_fan_val = &_pwm_analogWrite_val[_FAN_ID_MIN];

// a3ides heater output maximum values
const int *_heater_max = &_pwm_analogWrite_max[_HEATER_ID_MIN];
// a3ides heater output values
int *_heater_val = &_pwm_analogWrite_val[_HEATER_ID_MIN];

/*

// a3ides fan output maximum values
const int _fan_max[] = {255, 255};
// a3ides fan output values
int _fan_val[] = {0, 0};


// a3ides heater output maximum values
const int _heater_max[] = {255, 255};
// a3ides heater output values
int _heater_val[] = {0, 0};
*/
int hwio_jogwheel_enabled = 0;
int hwio_fan_control_enabled = 1;

float hwio_beeper_vol = 1.0F;
uint32_t hwio_beeper_del = 0;

extern ADC_HandleTypeDef hadc1;

/*****************************************************************************
 * private function declarations
 * */
void __pwm_set_val(TIM_HandleTypeDef *htim, uint32_t chan, int val);
void _hwio_pwm_analogWrite_set_val(int i_pwm, int val);
void _hwio_pwm_set_val(int i_pwm, int val);
uint32_t _pwm_get_chan(int i_pwm);
TIM_HandleTypeDef *_pwm_get_htim(int i_pwm);
int is_pwm_id_valid(int i_pwm);

//--------------------------------------
//analog output functions

int hwio_dac_get_cnt(void) //number of analog outputs
{ return _DAC_CNT; }

int hwio_dac_get_max(int i_dac) //analog output maximum value
{ return _dac_max[i_dac]; }

void hwio_dac_set_val(int i_dac, int val) //write analog output
{
}

//--------------------------------------
//pwm output functions

int is_pwm_id_valid(int i_pwm) {
    return ((i_pwm >= 0) && (i_pwm < static_cast<int>(_PWM_CNT)));
}

int hwio_pwm_get_cnt(void) //number of pwm outputs
{ return _PWM_CNT; }

int hwio_pwm_get_max(int i_pwm) //pwm output maximum value
{
    if (!is_pwm_id_valid(i_pwm))
        return -1;
    return _pwm_max[i_pwm];
}

//affects only prescaler
//affects multiple channels
void hwio_pwm_set_period_us(int i_pwm, int T_us) //set pwm resolution
{
    if (!is_pwm_id_valid(i_pwm))
        return;
    int *ptr_period_us = _pwm_period_us[i_pwm];

    if (T_us == *ptr_period_us)
        return;

    int prescaler = T_us * (int32_t)TIM_BASE_CLK_MHZ / (_pwm_max[i_pwm] + 1) - 1;
    hwio_pwm_set_prescaler(i_pwm, prescaler);
    //update seconds
    *ptr_period_us = T_us;
}

int hwio_pwm_get_period_us(int i_pwm) {
    if (!is_pwm_id_valid(i_pwm))
        return -1;
    return *_pwm_period_us[i_pwm];
}

void hwio_pwm_set_prescaler(int i_pwm, int prescaler) {
    if (hwio_pwm_get_prescaler(i_pwm) == prescaler)
        return;

    TIM_HandleTypeDef *htim = _pwm_get_htim(i_pwm);

    //uint32_t           chan = _pwm_get_chan(i_pwm);
    //uint32_t           cmp  = __HAL_TIM_GET_COMPARE(htim,chan);

    htim->Init.Prescaler = prescaler;
    //__pwm_set_val(htim, chan, cmp);

    __HAL_TIM_SET_PRESCALER(htim, prescaler);

    //calculate micro seconds
    int T_us = GEN_PERIOD_US(prescaler, htim->Init.Period);
    //update micro
    int *ptr_period_us = _pwm_period_us[i_pwm];
    *ptr_period_us = T_us;
}

int hwio_pwm_get_prescaler(int i_pwm) {
    if (!is_pwm_id_valid(i_pwm))
        return -1;
    TIM_HandleTypeDef *htim = _pwm_get_htim(i_pwm);
    return htim->Init.Prescaler;
}

//values should be:
//0000 0000 0000 0000 -exp = 0
//0000 0000 0000 0001 -exp = 1
//0000 0000 0000 0011 -exp = 2
//0000 0000 0000 0111 -exp = 3
//..
//0111 1111 1111 1111 -exp = 15
//1111 1111 1111 1111 -exp = 16

void hwio_pwm_set_prescaler_exp2(int i_pwm, int exp) {
    uint32_t prescaler = (1 << (exp)) - 1;
    hwio_pwm_set_prescaler(i_pwm, prescaler);
}

//reading value set by hwio_pwm_set_prescaler_exp2
int hwio_pwm_get_prescaler_log2(int i_pwm) {
    if (!is_pwm_id_valid(i_pwm))
        return -1;
    uint32_t prescaler = hwio_pwm_get_prescaler(i_pwm) + 1;
    int index = 0;

    while (prescaler != 0) {
        ++index;
        prescaler = prescaler >> 1;
    }

    return index - 1;
}

uint32_t _pwm_get_chan(int i_pwm) {
    if (!is_pwm_id_valid(i_pwm))
        return -1;
    return _pwm_chan[i_pwm];
}

TIM_HandleTypeDef *_pwm_get_htim(int i_pwm) {
    if (!is_pwm_id_valid(i_pwm))
        i_pwm = 0;

    return _pwm_p_htim[i_pwm];
}

void hwio_pwm_set_val(int i_pwm, uint32_t val) //write pwm output and update _pwm_analogWrite_val
{
    if (!is_pwm_id_valid(i_pwm))
        return;

    uint32_t chan = _pwm_get_chan(i_pwm);
    TIM_HandleTypeDef *htim = _pwm_get_htim(i_pwm);
    uint32_t cmp = __HAL_TIM_GET_COMPARE(htim, chan);

    if ((_pwm_analogWrite_val[i_pwm] ^ val) || (cmp != val)) {
        _hwio_pwm_set_val(i_pwm, val);

        //update _pwm_analogWrite_val
        int pwm_max = hwio_pwm_get_max(i_pwm);
        int pwm_analogWrite_max = _pwm_analogWrite_max[i_pwm];

        uint32_t pulse = (val * pwm_analogWrite_max) / pwm_max;
        _pwm_analogWrite_val[i_pwm] = pulse; //arduino compatible
    }
}

void _hwio_pwm_set_val(int i_pwm, int val) //write pwm output
{
    uint32_t chan = _pwm_get_chan(i_pwm);
    TIM_HandleTypeDef *htim = _pwm_get_htim(i_pwm);
    if ((chan == static_cast<uint32_t>(-1)) || htim->Instance == 0) {
        return;
    }

    __pwm_set_val(htim, chan, val);
}

void __pwm_set_val(TIM_HandleTypeDef *htim, uint32_t pchan, int val) //write pwm output
{
    if (htim->Init.Period) {
        TIM_OC_InitTypeDef sConfigOC = sConfigOC_default;
        if (val)
            sConfigOC.Pulse = val;
        else {
            sConfigOC.Pulse = htim->Init.Period;
            if (sConfigOC.OCPolarity == TIM_OCPOLARITY_HIGH)
                sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
            else
                sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        }
        if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, pchan) != HAL_OK) {
            Error_Handler();
        }
        HAL_TIM_PWM_Start(htim, pchan);
    } else
        HAL_TIM_PWM_Stop(htim, pchan);
}

void _hwio_pwm_analogWrite_set_val(int i_pwm, int val) {
    if (!is_pwm_id_valid(i_pwm))
        return;

    if (_pwm_analogWrite_val[i_pwm] != val) {
        int32_t pwm_max = hwio_pwm_get_max(i_pwm);
        uint32_t pulse = (val * pwm_max) / _pwm_analogWrite_max[i_pwm];
        hwio_pwm_set_val(i_pwm, pulse);
        _pwm_analogWrite_val[i_pwm] = val;
    }
}

//--------------------------------------
//fan control functions

int hwio_fan_get_cnt(void) //number of fans
{ return _FAN_CNT; }

void hwio_fan_set_pwm(int i_fan, int val) {
    i_fan += _FAN_ID_MIN;
    if ((i_fan >= _FAN_ID_MIN) && (i_fan <= _FAN_ID_MAX))
        _hwio_pwm_analogWrite_set_val(i_fan, val);
}

//--------------------------------------
// heater control functions

int hwio_heater_get_cnt(void) //number of heaters
{ return _HEATER_CNT; }

void hwio_heater_set_pwm(int i_heater, int val) {
    i_heater += _HEATER_ID_MIN;
    if ((i_heater >= _HEATER_ID_MIN) && (i_heater <= _HEATER_ID_MAX))
        _hwio_pwm_analogWrite_set_val(i_heater, val);
}

//--------------------------------------
// fan control - used for selftest

void hwio_fan_control_enable(void) {
    hwio_fan_control_enabled = 1;
}

void hwio_fan_control_disable(void) {
    hwio_fan_control_enabled = 0;
}

//--------------------------------------
// Jogwheel

void hwio_jogwheel_enable(void) {
    hwio_jogwheel_enabled = 1;
}

void hwio_jogwheel_disable(void) {
    hwio_jogwheel_enabled = 0;
}

//--------------------------------------
// Beeper

float hwio_beeper_get_vol(void) {
    return hwio_beeper_vol;
}

void hwio_beeper_set_vol(float vol) {
    if (vol < 0)
        vol *= -1;
    if (vol > 1)
        vol = 1;
    hwio_beeper_vol = vol;
}

void hwio_beeper_set_pwm(uint32_t per, uint32_t pul) {
    TIM_OC_InitTypeDef sConfigOC = { 0 };
    if (per) {
        htim2.Init.Prescaler = 0;
        htim2.Init.CounterMode = TIM_COUNTERMODE_DOWN;
        htim2.Init.Period = per;
        htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        HAL_TIM_Base_Init(&htim2);
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        if (pul) {
            sConfigOC.Pulse = pul;
            sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        } else {
            sConfigOC.Pulse = per;
            sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
        }
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    } else
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
}

void hwio_beeper_tone(float frq, uint32_t del) {
    uint32_t per;
    uint32_t pul;
    if (frq && del && hwio_beeper_vol) {
        if (frq < 0)
            frq *= -1;
        if (frq > 100000)
            frq = 100000;
        per = (uint32_t)(84000000.0F / frq);
        pul = (uint32_t)(per * hwio_beeper_vol / 2);
        hwio_beeper_set_pwm(per, pul);
        hwio_beeper_del = del;
    } else
        hwio_beeper_set_pwm(0, 0);
}

void hwio_beeper_tone2(float frq, uint32_t del, float vol) {
    hwio_beeper_set_vol(vol);
    hwio_beeper_tone(frq, del);
}

void hwio_beeper_notone(void) {
    hwio_beeper_set_pwm(0, 0);
}

void hwio_update_1ms(void) {
    if ((hwio_beeper_del) && ((--hwio_beeper_del) == 0))
        hwio_beeper_set_pwm(0, 0);
}

//--------------------------------------
// Arduino digital/analog read/write error handler

void hwio_arduino_error(int err, uint32_t pin32) {
    const int text_max_len = 64;
    char text[text_max_len];

    strlcat(text, "HWIO error\n", text_max_len);
    switch (err) {
    case HWIO_ERR_UNINI_DIG_RD:
    case HWIO_ERR_UNINI_DIG_WR:
    case HWIO_ERR_UNINI_ANA_RD:
    case HWIO_ERR_UNINI_ANA_WR:
        strlcat(text, "uninitialized\n", text_max_len);
        break;
    case HWIO_ERR_UNDEF_DIG_RD:
    case HWIO_ERR_UNDEF_DIG_WR:
    case HWIO_ERR_UNDEF_ANA_RD:
    case HWIO_ERR_UNDEF_ANA_WR:
        strlcat(text, "undefined\n", text_max_len);
        break;
    }

    snprintf(text + strlen(text), text_max_len - strlen(text),
        "pin #%i (0x%02x)\n", (int)pin32, (uint8_t)pin32);

    switch (err) {
    case HWIO_ERR_UNINI_DIG_RD:
    case HWIO_ERR_UNINI_DIG_WR:
    case HWIO_ERR_UNDEF_DIG_RD:
    case HWIO_ERR_UNDEF_DIG_WR:
        strlcat(text, "digital ", text_max_len);
        break;
    case HWIO_ERR_UNINI_ANA_RD:
    case HWIO_ERR_UNINI_ANA_WR:
    case HWIO_ERR_UNDEF_ANA_RD:
    case HWIO_ERR_UNDEF_ANA_WR:
        strlcat(text, "analog ", text_max_len);
        break;
    }

    switch (err) {
    case HWIO_ERR_UNINI_DIG_RD:
    case HWIO_ERR_UNDEF_DIG_RD:
    case HWIO_ERR_UNINI_ANA_RD:
    case HWIO_ERR_UNDEF_ANA_RD:
        strlcat(text, "read", text_max_len);
        break;
    case HWIO_ERR_UNINI_DIG_WR:
    case HWIO_ERR_UNDEF_DIG_WR:
    case HWIO_ERR_UNINI_ANA_WR:
    case HWIO_ERR_UNDEF_ANA_WR:
        strlcat(text, "write", text_max_len);
        break;
    }
    bsod(text);
}

/**
 * @brief Read digital pin to be used from Marlin
 *
 * Use MARLIN_PIN macro when handling special cases (virtual pins)
 * @example @code
 * case MARLIN_PIN(Z_MIN):
 * @endcode
 *
 * @todo Bypass electric signal when reading output pin
 */
int digitalRead(uint32_t marlinPin) {
#if _DEBUG
    if (!HAL_GPIO_Initialized) {
        hwio_arduino_error(HWIO_ERR_UNINI_DIG_RD, marlinPin); //error: uninitialized digital read
        return 0;
    }
#endif //_DEBUG
    switch (marlinPin) {
    default:
#if _DEBUG
        if (!buddy::hw::physicalPinExist(marlinPin)) {
            hwio_arduino_error(HWIO_ERR_UNDEF_DIG_RD, marlinPin); //error: undefined pin digital read
            return 0;
        }
#endif //_DEBUG
        return gpio_get(marlinPin);
    }
}

/**
 * @brief Write digital pin to be used from Marlin
 *
 * Use MARLIN_PIN macro when handling special cases (virtual pins)
 * @example @code
 * case MARLIN_PIN(FAN):
 * @endcode
 *
 */
void digitalWrite(uint32_t marlinPin, uint32_t ulVal) {
#if _DEBUG
    if (!HAL_GPIO_Initialized) {
        hwio_arduino_error(HWIO_ERR_UNINI_DIG_WR, marlinPin); //error: uninitialized digital write
        return;
    }
#endif //_DEBUG
    switch (marlinPin) {
    case MARLIN_PIN(BED_HEAT):
#ifdef SIM_HEATER_BED_ADC
        sim_bed_set_power(ulVal ? 100 : 0);
#else //SIM_HEATER_BED_ADC
        _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_BED, ulVal ? _pwm_analogWrite_max[HWIO_PWM_HEATER_BED] : 0);
#endif
        return;
    case MARLIN_PIN(HEAT0):
#ifdef SIM_HEATER_NOZZLE_ADC
        sim_nozzle_set_power(ulVal ? 40 : 0);
#else //SIM_HEATER_NOZZLE_ADC
        _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_0, ulVal ? _pwm_analogWrite_max[HWIO_PWM_HEATER_0] : 0);
#endif
        return;
    case MARLIN_PIN(FAN1):
#ifdef NEW_FANCTL
        if (hwio_fan_control_enabled)
            fanctl_set_pwm(1, ulVal ? (100 * 50 / 255) : 0);
#else  //NEW_FANCTL
        _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN1, ulVal ? 100 : 0);
#endif //NEW_FANCTL
        return;
    case MARLIN_PIN(FAN):
#ifdef NEW_FANCTL
        if (hwio_fan_control_enabled)
            fanctl_set_pwm(0, ulVal ? 50 : 0);
#else  //NEW_FANCTL
        _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN, ulVal ? _pwm_analogWrite_max[HWIO_PWM_FAN] : 0);
#endif //NEW_FANCTL
        return;
    default:
#if _DEBUG
        if (!buddy::hw::isOutputPin(marlinPin)) {
            hwio_arduino_error(HWIO_ERR_UNDEF_DIG_WR, marlinPin); //error: undefined pin digital write
            return;
        }
#endif //_DEBUG
        gpio_set(marlinPin, ulVal ? 1 : 0);
    }
}

uint32_t analogRead(uint32_t ulPin) {
    if (HAL_ADC_Initialized) {
        switch (ulPin) {
        case MARLIN_PIN(TEMP_BED):
            return get_adc_channel_value(&hadc1, CHANNEL_BED);
        case MARLIN_PIN(TEMP_0):
            return get_adc_channel_value(&hadc1, CHANNEL_NOZZLE);
        case MARLIN_PIN(TEMP_HEATBREAK):
            return get_adc_channel_value(&hadc1, CHANNEL_PINDA);
        case MARLIN_PIN(THERM2):
            return get_adc_channel_value(&hadc1, CHANNEL_TEMP_2);
        default:
            hwio_arduino_error(HWIO_ERR_UNDEF_ANA_RD, ulPin); //error: undefined pin analog read
        }
    } else
        hwio_arduino_error(HWIO_ERR_UNINI_ANA_RD, ulPin); //error: uninitialized analog read
    return 0;
}

void analogWrite(uint32_t ulPin, uint32_t ulValue) {
    if (HAL_PWM_Initialized) {
        switch (ulPin) {
        case MARLIN_PIN(FAN1):
            //hwio_fan_set_pwm(_FAN1, ulValue);
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN1, ulValue);
            return;
        case MARLIN_PIN(FAN):
            //hwio_fan_set_pwm(_FAN, ulValue);
#ifdef NEW_FANCTL
            if (hwio_fan_control_enabled)
                fanctl_set_pwm(0, ulValue * 50 / 255);
#else  //NEW_FANCTL
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN, ulValue);
#endif //NEW_FANCTL
            return;
        case MARLIN_PIN(BED_HEAT):
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_BED, ulValue);
            return;
        case MARLIN_PIN(HEAT0):
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_0, ulValue);
            return;
        default:
            hwio_arduino_error(HWIO_ERR_UNDEF_ANA_WR, ulPin); //error: undefined pin analog write
        }
    } else
        hwio_arduino_error(HWIO_ERR_UNINI_ANA_WR, ulPin); //error: uninitialized analog write
}

void pinMode(uint32_t ulPin, uint32_t ulMode) {
    // not supported, all pins are configured with Cube
}
