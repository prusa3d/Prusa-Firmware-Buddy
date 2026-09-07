/// @file
#include "hw_configuration.hpp"

#include "data_exchange.hpp"
#include "otp.hpp"
#include "timing_precise.hpp"
#include <adc.hpp>
#include <common/hwio_pindef.h>
#include <option/bootloader.h>
#include <option/has_door_sensor.h>
#include <option/has_indx_head.h>
#if HAS_DOOR_SENSOR()
    #include <buddy/door_sensor.hpp>
#endif

/*
+-----++-------------------------------------------------------------------------------+-------------------------+-------------------+
|pin  || MK4 xBuddy027                                                                 | MK4 xBuddy037           | MK3.5 xBuddy037   |
+-----++-------------------------------------------------------------------------------+-------------------------+-------------------+
|PA6  || Heatbreak temp                                                                | Heatbreak temp          | FILAMENT_SENSOR   |
|PA10 || accel CS                                                                      | accel CS                | TACHO0            |
|PE9  || FAN1 PWM                                                                      | FAN1 PWM inverted       | FAN1 PWM inverted |
|PE11 || FAN0 PWM                                                                      | FAN0 PWM inverted       | FAN0 PWM inverted |
|PE10 || both fans tacho / fan 0 tacho (old loveboard)                                 | both fans tacho         | TACHO1            |
|PE14 || NOT CONNECTED (requires R191 0R)  / fan 1 tacho (old loveboard + requires D6) | EXTRUDER_SWITCH         | EXTRUDER_SWITCH   |
|PF5  || -                                                                             | -                       | PINDA_THERM       | .. IGNORE
|PF13 || eeprom, fan multiplexor                                                       | eeprom, fan multiplexor | ZMIN (PINDA)      |
+-----++-------------------------------------------------------------------------------+-------------------------+-------------------+

LOVEBOARD support
need at least v31 (with EXTRUDER_SWITCH == multiplexer on fan tacho pins), xBuddy must have R191 0R instead of D6 to support 3V3 reference for LOVEBOARD
following table contains only differences with loveboard >= v31 and without old PINDA support (only SuperPINDA suported for MK3 extruder)
+-----++----------------------------------------------+-------------------------------+-------------------+
|pin  || MK4 xBuddy027 (with R191 0R)                 | MK4 xBuddy037                 | MK3.5 xBuddy037   |
+-----++----------------------------------------------+-------------------------------+-------------------+
|PA6  || Heatbreak temp                               | Heatbreak temp                | FILAMENT_SENSOR   |
|PA10 || accel CS                                     | accel CS                      | TACHO0            |
|PE9  || FAN1 PWM inverted                            | FAN1 PWM                      | FAN1 PWM          |
|PE10 || both fans tacho                              | both fans tacho               | TACHO1            |
|PE11 || FAN0 PWM inverted                            | FAN0 PWM                      | FAN0 PWM          |
|PE14 || NOT CONNECTED == same as MK4 on 037 (pullup) | EXTRUDER_SWITCH .. use pullup | EXTRUDER_SWITCH   |
|PF13 || eeprom, fan multiplexor                      | eeprom, fan multiplexor       | ZMIN (PINDA)      |
+-----++----------------------------------------------+-------------------------------+-------------------+


PC0 HOTEND_NTC is the same for all versions, but needs EXTRUDER_SWITCH enabled to provide pullup for MK3.5

xBuddy037 FW related changes
disconnected power panic cable will cause power panic
current measurement changed from 5V to 3V3 - need to recalculate values
MMU switching changed - no need to generate pulses anymore
MMU_RESET logic inverted
*/

namespace buddy::hw {

Configuration &Configuration::Instance() {
    static Configuration ths = Configuration();
    return ths;
}

Configuration::Configuration() {
    loveboard_bom_id = data_exchange::get_loveboard_eeprom().bomID;
    loveboard_present = data_exchange::get_loveboard_status().data_valid;
}

bool Configuration::check_bom_compatible() const {
    // Caution: This is BOM ID, not a board revision!
    const uint8_t board_bom_id = get_board_bom_id();

    // Note: Following dispatch should be based on BOARD_IS_*() macro but
    //       BOARD_IS_IXBUDDY() doesn't exist yet.
#if PRINTER_IS_PRUSA_iX()
    return board_bom_id >= 1;
#else
    if (board_bom_id >= 37) {
        // BOM ID 37 and newer are always compatible
        return true;
    }

    if (board_bom_id == 12 || board_bom_id == 14) {
        // We still need to support early revisions...
    #if PRINTER_IS_PRUSA_MK3_5()
        // ...but they are not compatible with MK3.5
        return false;
    #else
        return true;
    #endif
    }

    // Anything else is not compatible.
    return false;
#endif
}

bool Configuration::has_inverted_fans() const {
    return !PRINTER_IS_PRUSA_iX() && get_board_bom_id() < 37;
}

bool Configuration::has_mmu_power_up_hw() const {
    return PRINTER_IS_PRUSA_iX() || get_board_bom_id() >= 37;
}

bool Configuration::has_trinamic_oscillators() const {
    return PRINTER_IS_PRUSA_iX() || get_board_bom_id() >= 37;
}

bool Configuration::is_fw_compatible_with_hw() const {
#if PRINTER_IS_PRUSA_iX()
    return true;
#else

    #if HAS_DOOR_SENSOR()
    // MK4 also HAS_DOOR_SENSOR for HW compatibility check
    [[maybe_unused]] const bool door_sensor_connected = buddy::door_sensor().detailed_state().state != buddy::DoorSensor::State::sensor_detached;
    #endif /* HAS_DOOR_SENSOR() */

    #if !PRINTER_IS_PRUSA_MK3_5()
    // This procedure is checking if MK3.5 extruder is installed,
    // in which case we have an incompatible FW (MK4 | C1 | ...) and HW (MK3.5)
    // It is not possible to continue and info screen saying to reflash has to pop up
    const size_t count_of_validation_edges = 4;
    [[maybe_unused]] bool mk35_extruder_detected = true;
    for (size_t i = 0; i < count_of_validation_edges; ++i) {
        hx717Sck.write(Pin::State::low);
        delay_us_precise<1000>();
        if (hx717Dout.read() != Pin::State::low) {
            mk35_extruder_detected = false;
            break;
        }
        hx717Sck.write(Pin::State::high);
        delay_us_precise<1000>();
        if (hx717Dout.read() != Pin::State::high) {
            mk35_extruder_detected = false;
            break;
        }
    }
    #endif /* !PRINTER_IS_PRUSA_MK3_5() */

    // Disconnected thermistor should read 0x3ff value (10-bit adc channel with oversampling).
    // However, there are some 3V3 fluctuations so we allow changes on 2 least significant bits
    // When the thermistor is connected, we read values around 0x3D0
    constexpr uint32_t disconnected_bed_mask = 0b11'1111'1100;
    [[maybe_unused]] const bool bed_thermistor_connected = (AdcGet::bed() & disconnected_bed_mask) != disconnected_bed_mask;

    #if PRINTER_IS_PRUSA_COREONE()
    return door_sensor_connected && bed_thermistor_connected;
    #elif PRINTER_IS_PRUSA_COREONEL()
    return door_sensor_connected && !bed_thermistor_connected;
    #elif PRINTER_IS_PRUSA_MK4()
    return !door_sensor_connected && (loveboard_present || !mk35_extruder_detected);
    #elif PRINTER_IS_PRUSA_MK3_5()
    // valid data from loveboard means that we have MK4 HW, since MK3.5 does not have loveboard
    return !loveboard_present;
    #else
        #error
    #endif

#endif /* PRINTER_IS_PRUSA_iX() */
}

float Configuration::curr_measurement_voltage_to_current(float voltage) const {
    // Allegro ACS711KEXLT-15AB
    // +-15 A, 90mV/A, 0A -> output == Vcc/2
    // result in mA
    //
    // XBuddy 0.3.4 3V3 reference
    // XBuddy < 0.3.4 5V reference

    constexpr float allegro_curr_from_voltage = 1 / 0.09F;

    const float allegro_zero_curr_voltage = (get_board_bom_id() == 27) ? 5.F / 2.F : 3.35F / 2.F; // choose half of 3V3 or 5V range

    return (voltage - allegro_zero_curr_voltage) * allegro_curr_from_voltage;
}

#if PRINTER_IS_PRUSA_MK3_5()
// Definition intentionally missing, MK3.5 has no heatbreak thermistor at all
// so we want to get linker error if any code actually calls this.
#else
bool Configuration::needs_heatbreak_thermistor_table_5() const {
    return (loveboard_bom_id < 33 && loveboard_bom_id != 0) // error -> expect more common variant
        || loveboard_bom_id == 0xff; // error when run in simulator -> simulator uses table 5
}
#endif

bool Configuration::needs_software_mmu_powerup() const {
    // TODO: When we have a new bom this should be edited
    return true;
}

void Configuration::setup_ext_reset() const {
    const auto &config = Configuration::Instance();

    // Newer BOMs need push-pull for the reset pin, older open drain.
    // Setting it like this is a bit hacky, because the ext_reset defined in hwio_pindef is constexpr,
    // so it's not possible to change it right at the source.
    if (config.needs_push_pull_mmu_reset_pin()) {
        OutputPin pin = buddy::hw::ext_reset;
        pin.m_mode = OMode::pushPull;
        pin.configure();
    }
}

void Configuration::activate_ext_reset() const {
    ext_reset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::low : Pin::State::high);
}

void Configuration::deactivate_ext_reset() const {
    ext_reset.write(Configuration::Instance().has_inverted_mmu_reset() ? Pin::State::high : Pin::State::low);
}

bool Configuration::has_inverted_mmu_reset() const {
    return !PRINTER_IS_PRUSA_iX() && get_board_bom_id() >= 37;
}

bool Configuration::needs_push_pull_mmu_reset_pin() const {
    // xBuddy schematics says: Revisions older than 34 must use open drain only.
    return PRINTER_IS_PRUSA_iX() || get_board_bom_id() >= 34;
}

#if HAS_INDX_HEAD()
void Configuration::write_indx_head_reset(Pin::State state) const {
    // The reset is routed through the former FAN1 output stage on PE9, so it shares the fans' polarity quirk.
    // The indx_head_reset pin definition matches BOM >= 37, older boards need the opposite level.
    if (has_inverted_fans()) {
        state = (state == Pin::State::high) ? Pin::State::low : Pin::State::high;
    }
    indx_head_reset.write(state);
}
#endif

} // namespace buddy::hw
