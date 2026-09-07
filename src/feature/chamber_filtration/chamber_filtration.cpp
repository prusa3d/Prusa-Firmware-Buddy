#include "chamber_filtration.hpp"

#include <marlin_vars.hpp>
#include <marlin_server.hpp>
#include <gcode_info.hpp>
#include <tools_mapping.hpp>
#include <config_store/store_definition.hpp>
#include <bsod/bsod.h>

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
        return "DIY";
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
    debug_assert(osThreadGetId() == marlin_server::server_task);

    // The step acutally doesn't need to run often at all,
    // do not run it every marlin cycle
    const auto now_s = ticks_s();
    if (!step_rate_limiter_s_.check(now_s)) {
        return;
    }

    std::lock_guard _lg(mutex_);

    if (!is_enabled()) {
        output_pwm_ = {};
        last_filtration_need_s_ = std::nullopt;
        return;
    }

    // Determine output PWM of the fans
    if (needs_filtration()) {
        // Filtration is currently needed
        output_pwm_ = config_store().chamber_print_filtration_enable.get() ? config_store().chamber_mid_print_filtration_pwm.get() : PWM255(0);
        last_filtration_need_s_ = now_s;

    } else if (last_filtration_need_s_.has_value() && config_store().chamber_post_print_filtration_enable.get() && ticks_diff(now_s, *last_filtration_need_s_) <= config_store().chamber_post_print_filtration_duration_min.get() * 60) {
        // Filtration is not currently needed, running post print filtration
        output_pwm_ = config_store().chamber_post_print_filtration_pwm.get();

    } else {
        output_pwm_ = {};
        last_filtration_need_s_ = std::nullopt;
    }

    const auto commit_unaccounted_filter_usage = [&](int min_s = 1) {
        const auto unnacounted_usage_s = ticks_diff(now_s, unaccounted_filter_time_used_start_s_);
        if (unnacounted_usage_s < min_s) {
            return;
        }

        config_store().chamber_filter_time_used_s.apply([&](auto &val) { val += unnacounted_usage_s; });
        unaccounted_filter_time_used_start_s_ = now_s;
    };

    // If output_pwm > 0, track filter usage
    if (output_pwm_.value == 0) {
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

    bsod_unreachable();
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
        bsod_unreachable();
    }
}

bool ChamberFiltration::needs_filtration() const {
    // If explicitly set to false, we will never filter, so return early
    // If explicitly set to true, that will still depend on whether the nozzle is hot or not
    if (needs_filtration_override_ == Tristate::no) {
        return false;
    }

    // Check the always on flag (applies to all prints) [BFW-6829]
    const bool always_filter = config_store().chamber_filtration_always_on.get() || (needs_filtration_override_ == Tristate::yes);

    for (auto virtual_tool : VirtualToolIndex::all()) {
        const auto hotend_temp = marlin_vars().hotend(virtual_tool.to_physical()).temp_nozzle.get();

        // Save on config store lookups if the nozzle is cold
        if (hotend_temp <= 70) {
            continue;
        }

        const FilamentTypeParameters filament = FilamentType::for_tool_heuristic(virtual_tool).parameters();

        const bool hotend_is_hot =
            // Give some headroom over preheat temperature, we don't want the filtration to trigger when oscilating around it
            (hotend_temp > filament.nozzle_preheat_temperature + 5)

            // Backup in case the filament has nozzle_preheat_temperature set up weirdly
            || (hotend_temp >= filament.nozzle_temperature - 5);

        if (hotend_is_hot && (filament.requires_filtration || always_filter)) {
            return true;
        }
    }

    return false;
}

void ChamberFiltration::set_needs_filtration_override(Tristate set) {
    std::lock_guard _lg(mutex_);
    needs_filtration_override_ = set;
}

} // namespace buddy
