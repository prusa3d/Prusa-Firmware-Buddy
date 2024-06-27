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
#include "main.h"
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "fanctl.hpp"
#include "appmain.hpp"
#include "Marlin.h"
#include "MarlinPin.hpp"
#include <option/has_puppies.h>
#include <option/has_loadcell.h>
#include <option/has_gui.h>
#include <option/debug_with_beeps.h>
#include <option/is_knoblet.h>
#include "Marlin/src/module/motion.h" // for active_extruder
#include "puppies/modular_bed.hpp"
#include "otp.hpp"
#include "logging/log.h"

LOG_COMPONENT_REF(Buddy);

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "Marlin/src/module/prusa/toolchanger.h"
#endif
#if ENABLED(MODULAR_HEATBED)
    #include "Marlin/src/module/modular_heatbed.h"
#endif

#if !BOARD_IS_XLBUDDY
    #error This file is for XL buddy only
#endif

#if HAS_PUPPIES()
    #include "puppies/Dwarf.hpp"
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
const OutputPin *SideLed_LcdSelector = nullptr;
const OutputPin *XStep = nullptr;
const OutputPin *YStep = nullptr;
} // namespace buddy::hw

// stores board bom ID from OTP for faster access
uint8_t board_bom_id;

static float hwio_beeper_vol = 1.0F;
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

void hwio_beeper_tone(float frq, uint32_t del) {
    if (frq && del && hwio_beeper_vol) {
        if (frq < 0) {
            frq *= -1;
        }
        if (frq > 100000) {
            frq = 100000;
        }
        // Note: The frequency here is still too low for playing some common
        //       tunes with M300. We will have to find a free timer to use for
        //       updating buzzer pin, or hijack some already existing timer.
        constexpr const float hwio_beeper_frequency_hz = 1000.0f;
        hwio_beeper_pulses = del * frq / hwio_beeper_frequency_hz;
        hwio_beeper_period = hwio_beeper_frequency_hz / frq;
    } else {
        hwio_beeper_pulses = 0;
    }
}

void hwio_beeper_tone2(float frq, uint32_t del, float vol) {
    hwio_beeper_set_vol(vol);
    hwio_beeper_tone(frq, del);
}

void hwio_beeper_notone(void) {
    hwio_beeper_pulses = 0;
}

void hwio_update_1ms(void) {
#if HAS_GUI() && (DEBUG_WITH_BEEPS() || !_DEBUG)
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
        return buddy::puppies::dwarfs[0].is_tmc_enabled();
    case MARLIN_PIN(E1_ENA):
        return buddy::puppies::dwarfs[1].is_tmc_enabled();
    case MARLIN_PIN(E2_ENA):
        return buddy::puppies::dwarfs[2].is_tmc_enabled();
    case MARLIN_PIN(E3_ENA):
        return buddy::puppies::dwarfs[3].is_tmc_enabled();
    case MARLIN_PIN(E4_ENA):
        return buddy::puppies::dwarfs[4].is_tmc_enabled();
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
    switch (marlinPin) {
    case MARLIN_PIN(E0_ENA): {
        if (buddy::puppies::dwarfs[0].is_enabled()) {
            buddy::puppies::dwarfs[0].tmc_set_enable(ulVal);
        }
        break;
    }
    case MARLIN_PIN(E1_ENA): {
        if (buddy::puppies::dwarfs[1].is_enabled()) {
            buddy::puppies::dwarfs[1].tmc_set_enable(ulVal);
        }
        break;
    }
    case MARLIN_PIN(E2_ENA): {
        if (buddy::puppies::dwarfs[2].is_enabled()) {
            buddy::puppies::dwarfs[2].tmc_set_enable(ulVal);
        }
        break;
    }
    case MARLIN_PIN(E3_ENA): {
        if (buddy::puppies::dwarfs[3].is_enabled()) {
            buddy::puppies::dwarfs[3].tmc_set_enable(ulVal);
        }
        break;
    }
    case MARLIN_PIN(E4_ENA): {
        if (buddy::puppies::dwarfs[4].is_enabled()) {
            buddy::puppies::dwarfs[4].tmc_set_enable(ulVal);
        }
        break;
    }
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
        case MARLIN_PIN(FAN): // print fan
            Fans::print(active_extruder).setPWM(ulValue);
            buddy::puppies::modular_bed.set_print_fan_active(ulValue > 0);
            return;
        case MARLIN_PIN(FAN1):
            // heatbreak fan, any writes to it are ignored, its controlled by dwarf
            return;
        default:
            hwio_arduino_error(HWIO_ERR_UNDEF_ANA_WR, ulPin); // error: undefined pin analog write
        }
    } else {
        hwio_arduino_error(HWIO_ERR_UNINI_ANA_WR, ulPin); // error: uninitialized analog write
    }
}

void pinMode([[maybe_unused]] uint32_t ulPin, [[maybe_unused]] uint32_t ulMode) {
    // not supported, all pins are configured with Cube
}

void buddy::hw::hwio_configure_board_revision_changed_pins() {
    auto otp_bom_id = otp_get_bom_id();

    if (!otp_bom_id || (board_bom_id = *otp_bom_id) < 4) {
        if constexpr (option::is_knoblet) {
            board_bom_id = BOARD_VERSION_MINOR; // Knoblets can be without OTP (buzzer might not work)
        } else {
            bsod("Unable to determine board BOM ID");
        }
    }
    log_info(Buddy, "Detected bom ID %d", board_bom_id);

    // Different HW revisions have different pins connections, figure it out here
    if (board_bom_id >= 9 || board_bom_id == 4) {
        Buzzer = &pin_a0;
        XStep = &pin_d7;
        YStep = &pin_d5;
    } else {
        Buzzer = &pin_d5;
        XStep = &pin_a0;
        YStep = &pin_a3;
    }

    if (board_bom_id >= 9) {
        SideLed_LcdSelector = &pin_e9;
    }
}

void hw_init_spi_side_leds() {
    // Side leds was connectet to dedicated SPI untill revision 8, in revision 9 SPI is shared with LCD. So init SPI only if needed.
    if (board_bom_id <= 8) {
        hw_spi4_init();
    }
}
SPI_HandleTypeDef *hw_get_spi_side_strip() {
    if (board_bom_id >= 9 || board_bom_id == 4) {
        return &SPI_HANDLE_FOR(lcd);
    } else {
        return &hspi4;
    }
}
