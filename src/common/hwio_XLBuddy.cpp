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
#include "bsod.h"
#include <buddy/main.h>
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "fanctl.hpp"
#include "appmain.hpp"
#include "Marlin.h"
#include "MarlinPin.hpp"
#include <option/has_dwarf.h>
#include <option/has_loadcell.h>
#include <option/has_gui.h>
#include <option/debug_with_beeps.h>
#include "Marlin/src/module/motion.h" // for active_extruder
#include "puppies/modular_bed.hpp"
#include "hw_configuration.hpp"
#include <logging/log.hpp>

LOG_COMPONENT_REF(Buddy);

#if !BOARD_IS_XLBUDDY()
    #error This file is for XL buddy only
#endif

#if HAS_DWARF()
    #include "puppies/Dwarf.hpp"
    #include "puppies/PuppyModbus.hpp"
#endif

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

namespace buddy::hw {
const OutputPin *Buzzer = nullptr;
const OutputPin *XStep = nullptr;
const OutputPin *YStep = nullptr;
const OutputPin *SideLed_LcdSelector = nullptr;
} // namespace buddy::hw

static float hwio_beeper_vol = 1.0F;
static uint32_t hwio_beeper_duration_ms = 0;
// Needed for older XL boards, where SW buzzer control must be used
static std::atomic<uint32_t> hwio_beeper_pulses = 0;
static uint32_t hwio_beeper_period = 0;

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

void hwio_beeper_tone(float frq, uint32_t duration_ms) {
    if (frq && duration_ms && hwio_beeper_vol) {
        if (frq < 0) {
            frq *= -1;
        }
        if (frq > 100000) {
            frq = 100000;
        }

        if (buddy::hw::Configuration::Instance().has_alternative_board_pinout()) {
#if HAS_GUI() && (DEBUG_WITH_BEEPS() || !_DEBUG)
            uint32_t per = (uint32_t)(84000000.0F / frq);
            uint32_t pul = (uint32_t)(per * hwio_beeper_vol / 2);
            hwio_beeper_set_pwm(per, pul);
#endif
            hwio_beeper_duration_ms = duration_ms;
        } else {
            // SW control for older board revisions Buzzer PD5 pin
            // Note: The frequency here is still too low for playing some common
            //       tunes with M300. On PD5 pin there is no timer connected
            constexpr const float hwio_beeper_frequency_hz = 1000.0f;
            hwio_beeper_pulses = static_cast<uint32_t>(duration_ms * frq / hwio_beeper_frequency_hz);
            hwio_beeper_period = static_cast<uint32_t>(hwio_beeper_frequency_hz / frq);
        }
    } else {
        hwio_beeper_notone();
    }
}

void hwio_beeper_tone2(float frq, uint32_t duration_ms, float vol) {
    hwio_beeper_set_vol(vol);
    hwio_beeper_tone(frq, duration_ms);
}

void hwio_beeper_notone(void) {
    if (buddy::hw::Configuration::Instance().has_alternative_board_pinout()) {
        hwio_beeper_set_pwm(0, 0);
    } else {
        // SW control for older board revisions Buzzer PD5 pin
        hwio_beeper_pulses = 0;
    }
}

void hwio_update_1ms(void) {
#if HAS_GUI() && (DEBUG_WITH_BEEPS() || !_DEBUG)
    if (buddy::hw::Configuration::Instance().has_alternative_board_pinout()) {
        if ((hwio_beeper_duration_ms) && ((--hwio_beeper_duration_ms) == 0)) {
            hwio_beeper_set_pwm(0, 0);
        }
    } else {
        // SW control for older board revisions Buzzer PD5 pin
        static uint32_t skips = 0;
        if (skips < hwio_beeper_period - 1) {
            skips++;
            return;
        } else {
            skips = 0;
        }

        if (hwio_beeper_pulses > 0) {
            if (hwio_beeper_pulses % 2) {
                buddy::hw::Buzzer->reset();
            } else {
                buddy::hw::Buzzer->set();
            }
            hwio_beeper_pulses -= 1;
        }
    }
#endif
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
        return static_cast<bool>(buddy::hw::zMin.read());
    case MARLIN_PIN(XY_PROBE):
        return static_cast<bool>(buddy::hw::xyProbe.read());
#endif
    case MARLIN_PIN(E0_ENA):
        return buddy::puppies::dwarfs[PhysicalToolIndex::from_raw(0)].is_tmc_enabled();
    case MARLIN_PIN(E1_ENA):
        return buddy::puppies::dwarfs[PhysicalToolIndex::from_raw(1)].is_tmc_enabled();
    case MARLIN_PIN(E2_ENA):
        return buddy::puppies::dwarfs[PhysicalToolIndex::from_raw(2)].is_tmc_enabled();
    case MARLIN_PIN(E3_ENA):
        return buddy::puppies::dwarfs[PhysicalToolIndex::from_raw(3)].is_tmc_enabled();
    case MARLIN_PIN(E4_ENA):
        return buddy::puppies::dwarfs[PhysicalToolIndex::from_raw(4)].is_tmc_enabled();
    case MARLIN_PIN(X_STEP):
        return buddy::hw::XStep->read() == Pin::State::low ? 0 : 1;
    case MARLIN_PIN(Y_STEP):
        return buddy::hw::YStep->read() == Pin::State::low ? 0 : 1;
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
    const auto enable_tmc = [ulVal](PhysicalToolIndex tool) {
        auto dwarf = &buddy::puppies::dwarfs[tool];
        if (dwarf->is_enabled()) {
            /// TODO do not access puppyModbus outside of puppy task
            /// BFW-8185
            dwarf->tmc_set_enable(buddy::puppies::puppyModbus, ulVal);
        }
    };
    switch (marlinPin) {
    case MARLIN_PIN(E0_ENA):
        enable_tmc(PhysicalToolIndex::from_raw(0));
        break;
    case MARLIN_PIN(E1_ENA):
        enable_tmc(PhysicalToolIndex::from_raw(1));
        break;
    case MARLIN_PIN(E2_ENA):
        enable_tmc(PhysicalToolIndex::from_raw(2));
        break;
    case MARLIN_PIN(E3_ENA):
        enable_tmc(PhysicalToolIndex::from_raw(3));
        break;
    case MARLIN_PIN(E4_ENA):
        enable_tmc(PhysicalToolIndex::from_raw(4));
        break;
    case MARLIN_PIN(X_STEP):
        buddy::hw::XStep->write(ulVal ? Pin::State::high : Pin::State::low);
        break;
    case MARLIN_PIN(Y_STEP):
        buddy::hw::YStep->write(ulVal ? Pin::State::high : Pin::State::low);
        break;
    case MARLIN_PIN(DUMMY): {
        break;
    }
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
        case MARLIN_PIN(DUMMY):
            return 0;
        case MARLIN_PIN(THERM2):
            return AdcGet::boardTemp();
        case MARLIN_PIN(AMBIENT):
            return AdcGet::ambientTemp();
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
        case MARLIN_PIN(FAN): { // print fan
            if (auto tool = PhysicalToolIndex::currently_selected_opt()) {
                Fans::print(*tool).set_pwm(ulValue);
            }
            buddy::puppies::modular_bed.set_print_fan_active(ulValue > 0);
            return;
        }

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
    const auto &config = buddy::hw::Configuration::Instance();

    // Different HW revisions have different pins connections, figure it out here
    if (config.has_alternative_board_pinout()) {
        Buzzer = &buzzer_pin_a0;
        XStep = &xStep_pin_d7;
        YStep = &yStep_pin_d5;
    } else {
        Buzzer = &buzzer_pin_d5;
        XStep = &xStep_pin_a0;
        YStep = &yStep_pin_a3;
    }
    Buzzer->configure();
    XStep->configure();
    YStep->configure();

    if (config.has_shared_side_led_spi()) {
        SideLed_LcdSelector = &sideLed_LcdSelector_pin_e9;
        SideLed_LcdSelector->configure();
    }

    if (config.has_alternative_board_pinout()) {
        hw_tim2_init(); // TIM2 is used to generate buzzer PWM, except on older board revisions
    }
}

void hw_init_spi_side_leds() {
    if (!buddy::hw::Configuration::Instance().has_shared_side_led_spi()) {
        spi_init_led();
    }
}
SPI_HandleTypeDef *hw_get_spi_side_strip() {
    if (buddy::hw::Configuration::Instance().has_shared_side_led_spi()) {
        return spi_handle_lcd;
    } else {
        return spi_handle_led;
    }
}
