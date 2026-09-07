/**
 * @file
 * @brief hardware input output abstraction for Buddy board
 */

#include <inttypes.h>

#include "hwio.h"
#include "config.h"
#include <device/hal.h>
#include "cmsis_os.h"
#include "gpio.h"
#include "adc.hpp"
#include "Arduino.h"
#include "loadcell.hpp"
#include "timer_defaults.h"
#include "hwio_pindef.h"
#include <bsod.h>
#include <buddy/main.h>
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "fanctl.hpp"
#include "appmain.hpp"
#include "Marlin.h"
#include "MarlinPin.hpp"
#include <Marlin/src/module/temperature.h>

#include <option/has_loadcell.h>
#include <option/has_gui.h>
#include <option/has_local_bed.h>
#include <option/has_puppy_modularbed.h>
#include <option/has_indx.h>

#if PRINTER_IS_PRUSA_iX() && HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

#if HAS_PUPPY_MODULARBED()
    #include <puppies/modular_bed.hpp>
#endif

#include <Pin.hpp>
#include <cmath>

namespace {
/**
 * @brief hwio Marlin wrapper errors
 */
enum {
    HWIO_ERR_UNINI_DIG_RD = 0x01, //!< uninitialized digital read
    HWIO_ERR_UNINI_DIG_WR, //!< uninitialized digital write
    HWIO_ERR_UNINI_ANA_RD, //!< uninitialized analog read
    HWIO_ERR_UNINI_ANA_WR, //!< uninitialized analog write
    HWIO_ERR_UNDEF_DIG_RD, //!< undefined pin digital read
    HWIO_ERR_UNDEF_DIG_WR, //!< undefined pin digital write
    HWIO_ERR_UNDEF_ANA_RD, //!< undefined pin analog write
    HWIO_ERR_UNDEF_ANA_WR, //!< undefined pin analog write
};

} // end anonymous namespace

static const TIM_OC_InitTypeDef sConfigOC_default = {
    TIM_OCMODE_PWM1, // OCMode
    0, // Pulse
    TIM_OCPOLARITY_HIGH, // OCPolarity
    TIM_OCNPOLARITY_HIGH, // OCNPolarity
    TIM_OCFAST_DISABLE, // OCFastMode
    TIM_OCIDLESTATE_RESET, // OCIdleState
    TIM_OCNIDLESTATE_RESET // OCNIdleState
};

// buddy pwm output maximum values  as arduino analogWrite
static constexpr int _pwm_analogWrite_max = 255;

static float hwio_beeper_vol = 1.0F;
static uint32_t hwio_beeper_duration_ms = 0;

static void hwio_pwm_set_val(TIM_HandleTypeDef *htim, uint32_t pchan, int val) {
    // assuming arguments to this function are valid
    volatile uint32_t *ccreg = nullptr;
    volatile uint8_t *ccenable = nullptr;
    switch (pchan) {
    case TIM_CHANNEL_1:
        ccreg = &(htim->Instance->CCR1);
        ccenable = BITBAND_PERIPH(&htim->Instance->CCER, TIM_CCER_CC1E_Pos);
        break;
    case TIM_CHANNEL_2:
        ccreg = &(htim->Instance->CCR2);
        ccenable = BITBAND_PERIPH(&htim->Instance->CCER, TIM_CCER_CC2E_Pos);
        break;
    case TIM_CHANNEL_3:
        ccreg = &(htim->Instance->CCR3);
        ccenable = BITBAND_PERIPH(&htim->Instance->CCER, TIM_CCER_CC3E_Pos);
        break;
    case TIM_CHANNEL_4:
        ccreg = &(htim->Instance->CCR4);
        ccenable = BITBAND_PERIPH(&htim->Instance->CCER, TIM_CCER_CC4E_Pos);
        break;
    default:
        bsod("pwm: invalid timer CC channel");
        break;
    }

    if (val) {
        *ccreg = val;
        *ccenable = 0x1;
    } else {
        *ccenable = 0;
    }
}

//--------------------------------------
// Beeper

float hwio_beeper_get_vol(void) {
    return hwio_beeper_vol;
}

void hwio_beeper_set_vol(float vol) {
    if (vol < 0) {
        vol *= -1;
    }
    if (vol > 1) {
        vol = 1;
    }
    hwio_beeper_vol = vol;
}

#if HAS_GUI()
void hwio_beeper_set_pwm(uint32_t per, uint32_t pul) {
    TIM_OC_InitTypeDef sConfigOC {};
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
    } else {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    }
}
#endif

void hwio_beeper_tone(float frq, uint32_t duration_ms) {
    uint32_t per;
    uint32_t pul;
    if (frq && duration_ms && hwio_beeper_vol) {
        if (frq < 0) {
            frq *= -1;
        }
        if (frq > 100000) {
            frq = 100000;
        }
        per = (uint32_t)(84000000.0F / frq);
        pul = (uint32_t)(per * hwio_beeper_vol / 2);
        hwio_beeper_set_pwm(per, pul);
        hwio_beeper_duration_ms = duration_ms;
    } else {
        hwio_beeper_notone();
    }
}

void hwio_beeper_tone2(float frq, uint32_t duration_ms, float vol) {
    hwio_beeper_set_vol(vol);
    hwio_beeper_tone(frq, duration_ms);
}

void hwio_beeper_notone(void) {
    hwio_beeper_set_pwm(0, 0);
}

void hwio_update_1ms(void) {
    if ((hwio_beeper_duration_ms) && ((--hwio_beeper_duration_ms) == 0)) {
        hwio_beeper_set_pwm(0, 0);
    }
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
        hwio_arduino_error(HWIO_ERR_UNINI_DIG_RD, marlinPin); // error: uninitialized digital read
        return 0;
    }
#endif //_DEBUG
    switch (marlinPin) {
#if HAS_LOADCELL()
    case MARLIN_PIN(Z_MIN):
        if (Z_MIN_ENDSTOP_INVERTING) {
            return !loadcell.GetMinZEndstop() && static_cast<bool>(buddy::hw::zDiag.read());
        } else {
            return loadcell.GetMinZEndstop() || static_cast<bool>(buddy::hw::zDiag.read());
        }
    #if HAS_INDX()
    case MARLIN_PIN(XY_PROBE):
        if (XY_PROBE_ENDSTOP_INVERTING) {
            return !loadcell.GetXYEndstop();
        } else {
            return loadcell.GetXYEndstop();
        }
    #endif
#else
    case MARLIN_PIN(Z_MIN):
        return static_cast<bool>(buddy::hw::zMin.read());
#endif
    default:
#if _DEBUG
        if (!buddy::hw::physicalPinExist(marlinPin)) {
            hwio_arduino_error(HWIO_ERR_UNDEF_DIG_RD, marlinPin); // error: undefined pin digital read
            return 0;
        }
#endif //_DEBUG
        return gpio_get(marlinPin);
    }
}

#if HAS_LOCAL_BED()
void analogWrite_HEATER_BED(uint32_t new_value) {
    thermalManager.bed_pwm = new_value;

    static uint32_t old_value = 0;
    if (old_value != new_value) {
        static_assert(TIM3_default_Period == _pwm_analogWrite_max);
        hwio_pwm_set_val(&htim3, TIM_CHANNEL_3, new_value);
        old_value = new_value;
    }
}

uint32_t analogRead_TEMP_BED() {
    return AdcGet::bed();
}

#endif

static void analogWrite_HEATER_0(int new_value) {
    thermalManager.nozzle_pwm = new_value;

    static int old_value = 0;
    if (old_value != new_value) {
        static_assert(TIM3_default_Period == _pwm_analogWrite_max);
        hwio_pwm_set_val(&htim3, TIM_CHANNEL_4, new_value);
        old_value = new_value;
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
        hwio_arduino_error(HWIO_ERR_UNINI_DIG_WR, marlinPin); // error: uninitialized digital write
        return;
    }
#endif //_DEBUG
    switch (marlinPin) {
    case MARLIN_PIN(HEAT0):
        analogWrite_HEATER_0(ulVal ? _pwm_analogWrite_max : 0);
        return;

    case MARLIN_PIN(FAN):
        Fans::print(PhysicalToolIndex::from_raw(0)).set_pwm(ulVal ? 255 : 0);
        return;
    default:
#if _DEBUG
        if (!buddy::hw::isOutputPin(marlinPin)) {
            hwio_arduino_error(HWIO_ERR_UNDEF_DIG_WR, marlinPin); // error: undefined pin digital write
            return;
        }
#endif //_DEBUG
        gpio_set(marlinPin, ulVal ? 1 : 0);
    }
}

uint32_t analogRead(uint32_t ulPin) {
    if (HAL_ADC_Initialized) {
        switch (ulPin) {
        case MARLIN_PIN(TEMP_0):
#if (BOARD_IS_BUDDY() || BOARD_IS_XBUDDY())
            return AdcGet::nozzle();
#endif

#if (HAS_TEMP_HEATBREAK)
        case MARLIN_PIN(TEMP_HEATBREAK):
    #if (BOARD_IS_BUDDY())
            // #error dead code found by automatic analyses (see BFW-5461)
            return AdcGet::pinda();
    #elif (BOARD_IS_XBUDDY())
            return AdcGet::heatbreakTemp();
    #endif
#endif // HAS_TEMP_HEATBREAK

        case MARLIN_PIN(THERM2):
            return AdcGet::boardTemp();

#if PRINTER_IS_PRUSA_iX()
        case MARLIN_PIN(TEMP_PSU):
            return AdcGet::psu_temp();
        case MARLIN_PIN(TEMP_AMBIENT):
            return AdcGet::ambient_temp();
#endif

        default:
            hwio_arduino_error(HWIO_ERR_UNDEF_ANA_RD, ulPin); // error: undefined pin analog read
        }
    } else {
        hwio_arduino_error(HWIO_ERR_UNINI_ANA_RD, ulPin); // error: uninitialized analog read
    }
    return 0;
}

void analogWrite(uint32_t ulPin, uint32_t ulValue) {
    if (HAL_PWM_Initialized) {
        switch (ulPin) {
        case MARLIN_PIN(FAN):
            // The print-fan PWM mapping (e.g. HT high-temperature clamp) is applied centrally in
            // Temperature::manage_fans(), so every board's fan path gets it; here we just drive the pin.
            Fans::print(PhysicalToolIndex::from_raw(0)).set_pwm(ulValue);
#if HAS_PUPPY_MODULARBED()
            buddy::puppies::modular_bed.set_print_fan_active(ulValue > 0);
#endif
            return;
        case MARLIN_PIN(HEAT0):
            analogWrite_HEATER_0(ulValue);
            return;
        default:
            hwio_arduino_error(HWIO_ERR_UNDEF_ANA_WR, ulPin); // error: undefined pin analog write
        }
    } else {
        hwio_arduino_error(HWIO_ERR_UNINI_ANA_WR, ulPin); // error: uninitialized analog write
    }
}

void pinMode([[maybe_unused]] uint32_t ulPin, [[maybe_unused]] uint32_t ulMode) {
    // not supported, all pins are configured with stm32cube
}

void buddy::hw::hwio_configure_board_revision_changed_pins() {
    // no pins whose configuration changes between HW revisions on this board
}
