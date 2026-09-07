/// @file
#include <gui/screen/initial/screen_emergency_stop_consent.hpp>

#include <option/has_emergency_stop.h>
static_assert(HAS_EMERGENCY_STOP());

#include <config_store/store_instance.hpp>
#include <marlin_client.hpp>
#include <option/has_door_sensor_calibration.h>
static_assert(HAS_DOOR_SENSOR_CALIBRATION());

bool ScreenEmergencyStopConsent::should_show() {
    return !config_store().emergency_stop_enable.get()
        && !config_store().emergency_stop_disable_consent_given.get();
}

ScreenEmergencyStopConsent::ScreenEmergencyStopConsent()
    : PseudoScreenCallback {
        [] {
            // Check again - the user might have given the consent as part of the selftest snake
            if (should_show()) {
                // Run the door sensor calibration, only ask for the consent (and run the calibration)
                marlin_client::gcode("M1980 O");
            }
        },
    } {}
