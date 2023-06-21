#include "spool_join.hpp"
#include "Configuration_XL.h"
#include "log.h"
#include "marlin_server.hpp"
#include "module/motion.h"
#include "module/prusa/tool_mapper.hpp"
#include <cmath>
#include <limits>
#include <optional>
#include "module/prusa/toolchanger.h"
#include "module/temperature.h"
#include "module/planner.h" // for get_axis_position_mm
#include "marlin_vars.hpp"
#include "module/tool_change.h"
#include "lcd/extensible_ui/ui_api.h" // for ExtUI::onStatusChanged to send notification about spool join
#include "filament.hpp"               // for filament::set_type_in_extruder
#include <configuration_store.hpp>

SpoolJoin spool_join;

LOG_COMPONENT_REF(Marlin);

void SpoolJoin::reset() {
    num_joins = 0;
    for (auto &join : joins) {
        join.spool_1 = join.spool_2 = std::numeric_limits<uint8_t>::max();
    }
}

bool SpoolJoin::add_join(uint8_t spool_1, uint8_t spool_2) {
    if (num_joins >= joins.size() || !prusa_toolchanger.is_tool_enabled(spool_1) || !prusa_toolchanger.is_tool_enabled(spool_2) || spool_1 == spool_2)
        return false;

    // join will be added at the end of existing joins, so when for example
    // 0 will join with 1, and we want to join0 with 2,  actual join created will be 1 -> 2,
    // because we first want to join 0 -> 1, and then 1 -> 2
    for (auto &join : joins) {
        if (join.spool_1 == spool_1) {
            spool_1 = join.spool_2;
        }
    }

    // check again that we are not joining spool with itself - spool_1 might have changed above
    if (spool_1 == spool_2)
        return false;

    for (auto &join : joins) {
        if (join.spool_2 == spool_2) {
            // join to this spool was already configured before, do not allow to join to same spool twice
            return false;
        }
    }

    // save join
    join_config_t join;
    join.spool_1 = spool_1;
    join.spool_2 = spool_2;
    joins[num_joins++] = join;

    return true;
}

std::optional<uint8_t> SpoolJoin::get_join_for_tool(uint8_t tool) {
    for (size_t i = 0; i <= num_joins; i++) {
        if (joins[i].spool_1 == tool)
            return joins[i].spool_2;
    }

    return std::nullopt;
}

bool SpoolJoin::do_join(uint8_t current_tool) {
    auto join_settings = spool_join.get_join_for_tool(current_tool);
    if (!join_settings.has_value()) {
        return false;
    }

    uint8_t new_tool = join_settings.value();
    log_info(Marlin, "Spool join from %d to %d (z=%f)", current_tool, new_tool, planner.get_axis_position_mm(AxisEnum::Z_AXIS));

    ExtUI::onStatusChanged("Joining spool");

    planner.synchronize();

    xyze_pos_t return_pos = current_position;

#if DISABLED(SIGNLENOZZLE)

    // Park current tool, to get away from print
    tool_change(PrusaToolChanger::MARLIN_NO_TOOL_PICKED, tool_return_t::no_return);

    // transfer target temperature from one tool to another
    auto target_temp = thermalManager.degTargetHotend(current_tool);
    float display_temp = marlin_vars()->hotend(current_tool).display_nozzle;
    thermalManager.setTargetHotend(target_temp, new_tool);
    marlin_server::set_temp_to_display(display_temp, new_tool);

    // cool down current tool
    thermalManager.setTargetHotend(0, current_tool);
    marlin_server::set_temp_to_display(0, current_tool);

    // store that we have no filament in old nozzle
    config_store().set_filament_type(current_tool, filament::Type::NONE);
#endif

    // set up new tool mapping, so that next Tx will use spool we are joining to
    // but do mapping of logical->physical, so first convert current_tool to its logical tool
    if (!tool_mapper.set_mapping(tool_mapper.to_logical(current_tool), new_tool)) {
        return false;
    }
    tool_mapper.set_enable(true);

#if DISABLED(SINGLENOZZLE)
    if (target_temp != 0) {
        thermalManager.wait_for_hotend(new_tool, false, true);
    }
#endif

    // change to new tool
    destination = return_pos;
    tool_change(new_tool, tool_return_t::purge_and_to_destination);

    ExtUI::onStatusChanged("Spool joined");

    return true;
}
