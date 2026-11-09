#include "control_command.h"

#include <marlin_client.hpp>
#include <cstdio>
#include <state/printer_state.hpp>
#include <marlin_server_types/general_response.hpp>
#include <marlin_server_types/client_fsm_types.h>
#include <marlin_vars.hpp>

#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/chamber/chamber.hpp>
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
    #include <pwm_utils.hpp>
#endif

#include <option/has_side_leds.h>
#if HAS_SIDE_LEDS()
    #include <leds/side_strip_handler.hpp>
#endif

namespace nhttp::printer {

void ControlCommand::set_nozzle_temp(int temp) {
    char gcode[20];
    snprintf(gcode, sizeof(gcode), "M104 S%d", temp);
    marlin_client::gcode(gcode);
}

void ControlCommand::set_bed_temp(int temp) {
    char gcode[20];
    snprintf(gcode, sizeof(gcode), "M140 S%d", temp);
    marlin_client::gcode(gcode);
}

void ControlCommand::set_chamber_temp(int temp) {
#if HAS_XBUDDY_EXTENSION()
    if (temp <= 0) {
        buddy::chamber().set_target_temperature(std::nullopt);
    } else {
        buddy::chamber().set_target_temperature(temp);
    }
#else
    (void)temp;
#endif
}

void ControlCommand::set_speed(int percent) {
    char gcode[20];
    snprintf(gcode, sizeof(gcode), "M220 S%d", percent);
    marlin_client::gcode(gcode);
}

void ControlCommand::set_flow(int percent) {
    char gcode[20];
    snprintf(gcode, sizeof(gcode), "M221 S%d", percent);
    marlin_client::gcode(gcode);
}

void ControlCommand::set_fan_print(int pwm) {
    if (pwm <= 0) {
        marlin_client::gcode("M107");
    } else {
        char gcode[20];
        snprintf(gcode, sizeof(gcode), "M106 S%d", pwm > 255 ? 255 : pwm);
        marlin_client::gcode(gcode);
    }
}

void ControlCommand::set_chamber_fan(int percent) {
#if HAS_XBUDDY_EXTENSION()
    buddy::xbuddy_extension().set_fan_target_pwm(
        buddy::XBuddyExtension::Fan::cooling_fan_1,
        percent < 0
            ? buddy::XBuddyExtension::FanPWMOrAuto(pwm_auto)
            : buddy::XBuddyExtension::FanPWM::from_percent(percent > 100 ? 100 : percent));
#else
    (void)percent;
#endif
}

void ControlCommand::set_led(int percent) {
#if HAS_SIDE_LEDS()
    uint8_t brightness = static_cast<uint8_t>(percent > 100 ? 100 : (percent < 0 ? 0 : percent)) * 255 / 100;
    leds::SideStripHandler::instance().set_max_brightness(brightness);
    leds::SideStripHandler::instance().activity_ping();
#else
    (void)percent;
#endif
}

void ControlCommand::home_axes(bool x, bool y, bool z) {
    char gcode[20] = "G28";
    if (!(x && y && z)) {
        char *p = gcode + 3;
        if (x) { *p++ = ' '; *p++ = 'X'; }
        if (y) { *p++ = ' '; *p++ = 'Y'; }
        if (z) { *p++ = ' '; *p++ = 'Z'; }
        *p = '\0';
    }
    marlin_client::gcode(gcode);
}

void ControlCommand::relative_move(float x, float y, float z, float e, int feedrate) {
    marlin_client::gcode("G91");
    char gcode[80];
    snprintf(gcode, sizeof(gcode), "G1 F%d X%.2f Y%.2f Z%.2f E%.2f",
             feedrate, (double)x, (double)y, (double)z, (double)e);
    marlin_client::gcode(gcode);
    marlin_client::gcode("G90");
}

void ControlCommand::load_filament() {
    marlin_client::gcode("M701");
}

void ControlCommand::unload_filament() {
    marlin_client::gcode("M702");
}

void ControlCommand::purge() {
    marlin_client::gcode("M701 L0");
}

void ControlCommand::filament_change() {
    marlin_client::gcode("M600");
}

void ControlCommand::cooldown() {
    marlin_client::gcode("M104 S0");
    marlin_client::gcode("M140 S0");
    marlin_client::gcode("M107");
#if HAS_XBUDDY_EXTENSION()
    buddy::chamber().set_target_temperature(std::nullopt);
    buddy::xbuddy_extension().set_fan_target_pwm(
        buddy::XBuddyExtension::Fan::cooling_fan_1,
        buddy::XBuddyExtension::FanPWMOrAuto(pwm_auto));
#endif
}

void ControlCommand::motors_off() {
    marlin_client::gcode("M18");
}

void ControlCommand::emergency_stop() {
    marlin_client::gcode("M112");
}

void ControlCommand::reboot() {
    marlin_client::gcode("M997");
}

void ControlCommand::send_gcode(const char *gcode) {
    marlin_client::gcode(gcode);
}

void ControlCommand::dialog_response(const char *button_name) {
    Response resp = from_str(std::string_view(button_name));
    if (resp == Response::_none) {
        return; // Invalid button name
    }

    // Get current dialog state
    auto state = printer_state::get_state_with_dialog(false);
    if (!state.dialog.has_value()) {
        return; // No active dialog
    }

    // Get the current FSM top state for the response
    std::optional<fsm::States::Top> top;
    marlin_vars().peek_fsm_states([&](const auto &states) {
        top = states.get_top();
    });

    if (!top) {
        return;
    }

    marlin_client::FSM_encoded_response(EncodedFSMResponse {
        .response = FSMResponseVariant::make(resp),
        .fsm_and_phase = FSMAndPhase(top->fsm_type, top->data.GetPhase()),
    });
}

} // namespace nhttp::printer
