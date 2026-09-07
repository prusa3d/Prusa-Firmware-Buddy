/// @file
#include "hotend.hpp"

#include <module/temperature.h>
#include <tool/physical_tool.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "hotend/dummy_hotend.hpp"
#endif

Hotend &Hotend::for_tool(uint8_t tool) {
    return for_tool(PhysicalToolIndex::from_raw_notool(tool));
}

Hotend &Hotend::for_tool(PhysicalToolIndex tool) {
    return PhysicalTool::for_index(tool).hotend();
}

Hotend &Hotend::for_tool(std::variant<PhysicalToolIndex, NoTool> tool) {
    return PhysicalTool::for_index(tool).hotend();
}

PID_t Hotend::nozzle_pid_config_compat() const {
    const auto &pid = nozzle_pid_config();
    return PID_t {
        .Kp = pid.Kp,
        .Ki = pid.Ki,
        .Kd = pid.Kd,
    };
}

void Hotend::set_nozzle_pid_config_compat(const PID_t &set) {
    auto pid = nozzle_pid_config();
    pid.Kp = set.Kp;
    pid.Ki = set.Ki;
    pid.Kd = set.Kd;
    // Note: Keeping original kC, not covered by the heater sleftest
    set_nozzle_pid_config(pid);
}
