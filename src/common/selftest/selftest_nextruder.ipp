/// @file
/// Shared nozzle heater selftest configuration for Nextruder-based printers
/// (CORE One, CORE One L, including their INDX variants).
///
/// This file is intended to be included from a single per-printer .cpp file
/// (selftest_COREONE.cpp / selftest_COREONEL.cpp); it deliberately does not
/// have an include guard. The host file is responsible for the includes the
/// configs need (Hotend, Fans, HotendType, log, ...).

static constexpr HeaterConfig_t Config_HeaterNozzle_Standard[] = {
    {
        .partname = "Nozzle",
        .type = heater_type_t::Nozzle,
        .getTemp = []() {
            auto tool = PhysicalToolIndex::currently_selected_opt();
            if (!tool) {
                bsod("Heater config getTemp called without active tool");
            }
            return Hotend::for_tool(*tool).nozzle_temp(); },
        .setTargetTemp = [](int target_temp) {
            auto tool = PhysicalToolIndex::currently_selected_opt();
            if (!tool) {
                bsod("Heater config setTargetTemp called without active tool");
            }
            Hotend::for_tool(*tool).set_nozzle_target_temp(target_temp); },
        .get_pid = []() {
            auto tool = PhysicalToolIndex::currently_selected_opt();
            if (!tool) {
                bsod("Heater config get_pid called without active tool");
            }
            return Hotend::for_tool(*tool).nozzle_pid_config_compat(); },
        .set_pid = [](const PID_t &pid) {
            auto tool = PhysicalToolIndex::currently_selected_opt();
            if (!tool) {
                bsod("Heater config set_pid called without active tool");
            }
            Hotend::for_tool(*tool).set_nozzle_pid_config_compat(pid); },
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
#if HAS_INDX()
        // Unused by the gcode selftest — the INDX nozzle passes/fails by the thermal-protection
        // verdict (see heat_timeout_ms); kept for the legacy CSelftest code path.
        .heat_time_ms = 5700,
#else
        .heat_time_ms = 42000,
#endif
        .start_temp = 80,
        .undercool_temp = 75,
        .target_temp = 290,
#if HAS_INDX()
        // Unused by the gcode selftest — see heat_time_ms above.
        .heat_min_temp = 185,
        .heat_max_temp = 255,
#else
        .heat_min_temp = 195,
        .heat_max_temp = 245,
#endif
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .min_pwm_to_measure = 26,
#if HAS_INDX()
        .heat_timeout_ms = 60000,
#endif
    }
};

#if HAS_HT_HOTEND()
// HT hotend (PT1000): copper heater block has ~2x the thermal mass of the standard
// aluminium block, so the heating curve is materially slower. From recorded data
// (BFW-8665), reaching 80 °C → ~200 °C in 60 s on a slow unit, ~225 °C on a fast one.
// The window [170, 260] gives 30 °C / 35 °C of margin around that spread.
// Compiled only on HT-capable variants — INDX does not (and will not) carry an HT hotend.
static constexpr HeaterConfig_t Config_HeaterNozzle_HighTemp[] = {
    {
        .partname = "Nozzle",
        .type = heater_type_t::Nozzle,
        .getTemp = []() { return Hotend::for_tool(PhysicalToolIndex::from_raw(0)).nozzle_temp(); },
        .setTargetTemp = [](int target_temp) { Hotend::for_tool(PhysicalToolIndex::from_raw(0)).set_nozzle_target_temp(target_temp); },
        .get_pid = []() { return Hotend::for_tool(PhysicalToolIndex::from_raw(0)).nozzle_pid_config_compat(); },
        .set_pid = [](const PID_t &pid) { Hotend::for_tool(PhysicalToolIndex::from_raw(0)).set_nozzle_pid_config_compat(pid); },
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 60000,
        .start_temp = 80,
        .undercool_temp = 75,
        .target_temp = 290,
        .heat_min_temp = 170,
        .heat_max_temp = 260,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 55,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .min_pwm_to_measure = 26,
    }
};

static std::span<const HeaterConfig_t, 1> Config_HeaterNozzle() {
    // hotend_type comes from EEPROM. Only the HT hotend uses the high-temperature config;
    // every other value (a standard hotend, or a corrupted / future-FW value) uses the
    // standard, lower-temperature (305 C) config. This is deliberately graceful rather than
    // a closed-enum switch + bsod: EEPROM values are external input, and a downgrade or an
    // unknown value must not brick the printer (an expensive factory-reset recovery). The
    // standard config is over-conservative, not unsafe; a cold boot re-runs detection.
    if (config_store().hotend_type.get(0) == HotendType::high_temp) {
        return Config_HeaterNozzle_HighTemp;
    }
    return Config_HeaterNozzle_Standard;
}
#else
static std::span<const HeaterConfig_t, 1> Config_HeaterNozzle() {
    return Config_HeaterNozzle_Standard;
}
#endif
