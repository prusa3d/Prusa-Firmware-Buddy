#include "chamber_filtration.hpp"

#include <marlin_vars.hpp>
#include <marlin_server.hpp>
#include <gcode_info.hpp>
#include <tools_mapping.hpp>
#include <config_store/store_definition.hpp>

#include <feature/xbuddy_extension/xbuddy_extension.hpp>

namespace buddy {

ChamberFiltration &chamber_filtration() {
    static ChamberFiltration instance;
    return instance;
}

ChamberFiltrationBackend ChamberFiltration::backend() const {
    return config_store().chamber_filtration_backend.get();
}

const char *ChamberFiltration::backend_name(Backend backend) {
    switch (backend) {
    case Backend::none:
        return N_("None");

#if HAS_XBUDDY_EXTENSION()
    case Backend::xbe_official_filter:
        return N_("Adv. filtration");

    case Backend::xbe_filter_on_cooling_fans:
        return N_("DIY");
#endif
    }

    return nullptr;
}

size_t ChamberFiltration::get_available_backends(BackendArray &target) {
    size_t i = 0;

    const auto append = [&]<Backend b>() {
        static_assert(std::to_underlying(b) < max_backend_count);
        target[i++] = b;
    };

    append.operator()<Backend::none>();

#if HAS_XBUDDY_EXTENSION()
    if (xbuddy_extension().status() != XBuddyExtension::Status::disabled) {
        append.operator()<Backend::xbe_official_filter>();
        append.operator()<Backend::xbe_filter_on_cooling_fans>();
    }
#endif

    return i;
}

PWM255 ChamberFiltration::output_pwm() const {
    std::lock_guard _lg(mutex_);
    return output_pwm_;
}

void ChamberFiltration::step() {
    assert(osThreadGetId() == marlin_server::server_task);

    std::lock_guard _lg(mutex_);

    // Only start filtering after we've extruded first filament
    // We don't want the filtering fans to slow down the chamber heatup
    const bool is_printing = marlin_server::is_printing_state(marlin_vars().print_state.get()) /* && (planner.max_printed_z > 0) */ /* max_printed_z not merged in 6.3 */;
    const bool was_printing = is_printing_prev_;
    is_printing_prev_ = is_printing;

    if (is_printing && !was_printing) {
        // Checking is a bit expensive, do it only at the beginning of the print
        update_needs_filtration();
    }

    if (!needs_filtration_) {
        output_pwm_ = {};

    } else if (is_printing) {
        output_pwm_ = config_store().chamber_mid_print_filtration_pwm.get();
        last_print_s_ = ticks_s();

    } else if (config_store().chamber_post_print_filtration_enable.get() && ticks_diff(ticks_s(), last_print_s_) <= config_store().chamber_post_print_filtration_duration_min.get() * 60) {
        output_pwm_ = config_store().chamber_post_print_filtration_pwm.get();

    } else {
        output_pwm_ = {};
        needs_filtration_ = false;
    }
}

void ChamberFiltration::update_needs_filtration() {
    needs_filtration_ = false;

    EXTRUDER_LOOP() {
        const auto &extruder_info = GCodeInfo::getInstance().get_extruder_info(e);
        if (!extruder_info.used()) {
            continue;
        }

        const uint8_t tool_index = tools_mapping::to_physical_tool(e);
        if (tool_index == tools_mapping::no_tool) {
            continue;
        }

        const bool loaded_filament_requires_filtration = config_store().get_filament_type(tool_index).parameters().requires_filtration;

        const bool gcode_filament_requires_filtration = //
            extruder_info.filament_name.has_value()
            ? FilamentType::from_name(extruder_info.filament_name->data()).parameters().requires_filtration
            : false;

        if (loaded_filament_requires_filtration || gcode_filament_requires_filtration) {
            needs_filtration_ = true;
            return;
        }
    }
}

} // namespace buddy
