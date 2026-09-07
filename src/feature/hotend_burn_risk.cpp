/// @file
#include "hotend_burn_risk.hpp"

#include <option/has_ht_hotend.h>
#include <buddy/door_sensor.hpp>
#include <tool/hotend/hotend.hpp>
#include <marlin_server.hpp>

static_assert(HAS_HT_HOTEND());

namespace buddy {

void check_hotend_burn_risk() {
    // Burn risk: the door is physically open and the nozzle is dangerously hot.
    // Firmware-owned (no user button), dismissed automatically when either condition
    // clears — mirrors DoorOpen. Checks door_open (not !door_closed) so a
    // sensor_detached state does not raise a burn warning.
    // (When emergency stop is on and both fire, DoorOpen keeps display priority.)
    static_assert(PhysicalToolIndex::count == 1, "Burn risk check assumes a single physical tool");
    const bool is_door_open = door_sensor().state() == DoorSensor::State::door_open;
    const auto nozzle_temp = Hotend::for_tool(PhysicalToolIndex::from_raw(0)).nozzle_temp().value_or(0);
    constexpr auto hysteresis = 5;

    if (is_door_open && nozzle_temp >= Hotend::burn_warning_temp) {
        marlin_server::set_warning(WarningType::HotendBurnRisk);

    } else if (!is_door_open || nozzle_temp < Hotend::burn_warning_temp - hysteresis) {
        marlin_server::clear_warning(WarningType::HotendBurnRisk);
    }
}

} // namespace buddy
