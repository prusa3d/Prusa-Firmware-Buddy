#include "chamber_filtration.hpp"

#include <marlin_vars.hpp>
#include <marlin_server.hpp>
#include <gcode_info.hpp>
#include <tools_mapping.hpp>
#include <config_store/store_definition.hpp>
#include <buddy/unreachable.hpp>

#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

#include <option/xl_enclosure_support.h>
#if XL_ENCLOSURE_SUPPORT()
    #include <xl_enclosure.hpp>
#endif

namespace buddy {

ChamberFiltration &chamber_filtration() {
    static ChamberFiltration instance;
    return instance;
}

ChamberFiltrationBackend ChamberFiltration::backend() const {
    return config_store().chamber_filtration_backend.get();
}

void ChamberFiltration::set_backend(ChamberFiltrationBackend backend) {
    config_store().chamber_filtration_backend.set(backend);

#if XL_ENCLOSURE_SUPPORT()
    xl_enclosure.setEnabled(backend == ChamberFiltrationBackend::xl_enclosure);
#endif
}

const char *ChamberFiltration::backend_name(Backend backend) {
    switch (backend) {
    case Backend::none:
        return N_("None");

#if XL_ENCLOSURE_SUPPORT()
    case Backend::xl_enclosure:
        return N_("Enclosure");
#endif

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

#if XL_ENCLOSURE_SUPPORT()
    // We cannot know if enclosure is mounted until we enable it and test fan's RPM
    // For that, it has to be available
    append.operator()<Backend::xl_enclosure>();
#elif HAS_XBUDDY_EXTENSION()
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

    if (!is_enabled()) {
        output_pwm_ = {};
        is_printing_prev_ = false;
        needs_filtration_ = false;
        return;
    }

    // Only start filtering after we've extruded first filament
    // We don't want the filtering fans to slow down the chamber heatup
    const bool is_printing = marlin_server::is_printing_state(marlin_vars().print_state.get()) && (planner.max_printed_z > 0);
    const bool was_printing = is_printing_prev_;
    is_printing_prev_ = is_printing;

    if (is_printing && !was_printing) {
        // Checking is a bit expensive, do it only at the beginning of the print
        update_needs_filtration();
    }

    const auto now_s = ticks_s();

    // Determine output PWM of the fans
    if (!needs_filtration_.value_or(false)) {
        output_pwm_ = {};

    } else if (is_printing) {
        output_pwm_ = config_store().chamber_print_filtration_enable.get() ? config_store().chamber_mid_print_filtration_pwm.get() : PWM255(0);
        last_print_s_ = now_s;

    } else if (config_store().chamber_post_print_filtration_enable.get() && ticks_diff(now_s, last_print_s_) <= config_store().chamber_post_print_filtration_duration_min.get() * 60) {
        output_pwm_ = config_store().chamber_post_print_filtration_pwm.get();

    } else {
        output_pwm_ = {};
        needs_filtration_ = std::nullopt; // Reset the flag after the print is done so that it doesn't affect the next print
    }

    const auto commit_unaccounted_filter_usage = [&](int min_s = 1) {
        const auto unnacounted_usage_s = ticks_diff(now_s, unaccounted_filter_time_used_start_s_);
        if (unnacounted_usage_s < min_s) {
            return;
        }

        config_store().chamber_filter_time_used_s.apply([&](auto &val) { val += unnacounted_usage_s; });
        unaccounted_filter_time_used_start_s_ = now_s;
    };

    // Track wear by the actual fan PWM rather than the filtration request - on Core One the same fan also serves chamber cooling, so air can pass through the filter even when filtration itself didn't ask for it.
    const bool filter_active = [&] {
#if HAS_XBUDDY_EXTENSION()
        switch (backend()) {
        case Backend::xbe_official_filter:
            return xbuddy_extension().fan_actual_pwm(XBuddyExtension::Fan::filtration_fan).value > 0;
        case Backend::xbe_filter_on_cooling_fans:
            return xbuddy_extension().fan_actual_pwm(XBuddyExtension::Fan::cooling_fan_1).value > 0;
        default:
            break;
        }
#endif
        return output_pwm_.value > 0;
    }();

    if (!filter_active) {
        if (unaccounted_filter_time_used_start_s_) {
            // Commit any remaining unaccounted time
            commit_unaccounted_filter_usage();
            unaccounted_filter_time_used_start_s_ = 0;
        }

    } else if (unaccounted_filter_time_used_start_s_ == 0) {
        unaccounted_filter_time_used_start_s_ = now_s;

    } else {
        // Reduce eeprom writes - update filter usage in certain intervals
        commit_unaccounted_filter_usage(60);
    }
}

uint32_t ChamberFiltration::filter_lifetime_s() const {
    switch (backend()) {

    case Backend::none:
        return 0;
#if XL_ENCLOSURE_SUPPORT()
    case Backend::xl_enclosure:
        return 600 * 3600;
#endif
#if HAS_XBUDDY_EXTENSION()
    case Backend::xbe_official_filter:
        return 600 * 3600;

    case Backend::xbe_filter_on_cooling_fans:
        // DIY solution, unknown rated life. Let's say that it's the same as the official filter
        return 600 * 3600;
#endif
    }

    BUDDY_UNREACHABLE();
}

void ChamberFiltration::check_filter_expiration() {
    /// How much in advance (in filter time usage seconds) we should warn that the filter is about to expire
    static constexpr auto expiration_early_warning_s = 100 * 3600;

    const auto filter_lifetime_s = this->filter_lifetime_s();
    if (!filter_lifetime_s) {
        return;
    }

    const auto filter_time_used_s = config_store().chamber_filter_time_used_s.get();

    if (filter_time_used_s < filter_lifetime_s - expiration_early_warning_s) {
        // All is well, reset any warnings and postpones
        config_store().chamber_filter_expiration_postpone_timestamp_1024.set_to_default();
        config_store().chamber_filter_early_expiration_warning_shown.set_to_default();

    } else if (filter_time_used_s < filter_lifetime_s) {
        if (!config_store().chamber_filter_early_expiration_warning_shown.get()) {
            marlin_server::set_warning(WarningType::EnclosureFilterExpirWarning);
            config_store().chamber_filter_early_expiration_warning_shown.set(true);
        }

    } else {
        const auto current_time = time(nullptr);
        const auto postpone_time = config_store().chamber_filter_expiration_postpone_timestamp_1024.get();
        if (current_time / 1024 >= postpone_time) {
            marlin_server::set_warning(WarningType::EnclosureFilterExpiration);
        }
    }
}

void ChamberFiltration::change_filter() {
    config_store().chamber_filter_time_used_s.set(0);
    // Postpones and such get cleared in the next check_filter_expiration call
}

void ChamberFiltration::handle_filter_expiration_warning(Response response) {
    switch (response) {

    case Response::_none:
        break;

    case Response::Ignore:
        // Do nothing, show warning on next occasion
        break;

    case Response::Postpone5Days: {
        if (const auto current_time = time(nullptr); current_time > 0) {
            // Do nothing if the RTC clock is not set up
            config_store().chamber_filter_expiration_postpone_timestamp_1024.set((current_time + 5 * 24 * 3600) / 1024);
        }
        break;
    }

    case Response::Done:
        change_filter();
        break;

    default:
        BUDDY_UNREACHABLE();
    }
}

void ChamberFiltration::update_needs_filtration() {
    // Check the always on flag (applies to all prints) [BFW-6829]
    if (config_store().chamber_filtration_always_on.get()) {
        needs_filtration_ = true;
        return;
    }
    // Check if special gcode M147 or M148 overrides the setting (current print only) [BFW-6828]
    if (needs_filtration_.has_value()) {
        return;
    }
    needs_filtration_ = false;
    GCodeInfo::getInstance().for_each_used_extruder([this]([[maybe_unused]] uint8_t logical_ix, uint8_t tool_index, const GCodeInfo::ExtruderInfo &extruder_info) {
        const bool loaded_filament_requires_filtration = config_store().get_filament_type(tool_index).parameters().requires_filtration;

        const bool gcode_filament_requires_filtration = //
            extruder_info.filament_name.has_value()
            ? FilamentType::from_name(extruder_info.filament_name->data()).parameters().requires_filtration
            : false;

        if (loaded_filament_requires_filtration || gcode_filament_requires_filtration) {
            needs_filtration_ = true;
        }
    });
}

void ChamberFiltration::set_needs_filtration(bool needs_filtration) {
    std::lock_guard _lg(mutex_);
    needs_filtration_ = needs_filtration;
}

} // namespace buddy
