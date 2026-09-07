/// @file
#pragma once

#include <variant>
#include <inplace_function.hpp>

#include <common/hw_check.hpp>
#include <utils/storage/enum_bitset.hpp>
#include <common/marlin_server_types/general_response.hpp>
#include <bsod/bsod.h>

struct IWindowMenuItemColorScheme;
namespace img {
struct Resource;
}

namespace buddy::compatibility_checks {

/// !!! MUST NOT BE STORED IN EEPROM, ITEMS CAN CHANGE OR BE REORDERED
/// Sorted from the least severe to the most severe
enum class CompatibilityLevel : uint8_t {
    /// Everything is compatible
    fully_compatible = 0,

    /// Compatible, just pop up a reminder about something
    compatible_with_reminder,

    /// Not fully compatible, continue only with user approval
    needs_user_approval,

    /// Not compatible, cannot continue
    fatal_incompatibility,

    _last = fatal_incompatibility,
};

extern const EnumArray<CompatibilityLevel, const img::Resource *, std::to_underlying(CompatibilityLevel::_last) + 1> compatibility_level_icons;

extern const EnumArray<CompatibilityLevel, const IWindowMenuItemColorScheme *, std::to_underlying(CompatibilityLevel::_last) + 1> compatibility_level_menu_item_color_schemes;

struct CheckMetadata {
    /// Severity if the check fails
    /// Can either be a hardcoded severity or a HWCheckType with user-configurable severity
    std::variant<HWCheckSeverity, HWCheckType, CompatibilityLevel> severity;

    CompatibilityLevel evaluate_compatibility() const;

    /// Translatable error message, WITHOUT a trailing '.'
    const char *title;

    /// Translatable long message. Can provide further information. Written in full sentences.
    const char *description;
};

template <typename Check_>
struct ChecksTraits {
    using Check = Check_;

    using Metadata = const EnumArray<Check, CheckMetadata, Check::_cnt>;
    static Metadata metadata;

    using Bitset = EnumBitset<Check, Check::_cnt>;

    /// @returns false if the iteration should stop
    using Visitor = stdext::inplace_function<bool(const CheckMetadata &)>;

    /// @returns false if the iteration stopped by visitor returning false
    static bool visit_set_bits(const Bitset &bitset, const Visitor &visitor) {
        for (uint8_t i = 0; i < std::to_underlying(Check::_cnt); i++) {
            if (bitset.test(i)) {
                if (!visitor(metadata[i])) {
                    return false;
                }
            }
        }

        return true;
    }
};

/// Wrapped MsgBox to prevent header include pollution
void gui_incompatibility_error(const CheckMetadata &check, Response abort_response);

/// Wrapped MsgBox to prevent header include pollution
[[nodiscard]] bool gui_confirm_incompatibility_default(const CheckMetadata &check, Response abort_response);

/// Curiously-recurring template for compatibility reports
template <typename Report>
struct CompatibilityReportBase {
    /// @returns (first) failed check of the highest severity
    /// @param check_filter_f only considers failed checks that match the filter
    auto highest_severity_failed_check_filtered(auto check_filter_f) const {
        struct {
            std::optional<typename Report::FailedCheck> check;
            CompatibilityLevel severity = CompatibilityLevel::fully_compatible;
        } result;

        static_cast<const Report *>(this)->visit_failed_checks([&](const Report::FailedCheck &check) {
            if (!check_filter_f(check)) {
                return true;
            }

            const auto severity = check.meta->evaluate_compatibility();
            if (!result.check.has_value() || result.severity < severity) {
                result = { check, severity };
            }

            return true;
        });

        return result.check;
    }

    auto highest_severity_failed_check() const {
        return highest_severity_failed_check_filtered([](const Report::FailedCheck &) -> bool { return true; });
    }

    /// Severity of the failures
    CompatibilityLevel compatibility_level() const {
        const auto check = highest_severity_failed_check();
        if (!check) {
            return CompatibilityLevel::fully_compatible;
        }

        return check->meta->evaluate_compatibility();
    }

    /// If there is a failed check with abort severity, shows that one.
    /// Otherwise shows a warning for each failed check with the warning severity.
    /// The user needs to confirm ignoring all of the warnings.
    /// Some warning ignores MAY change printer state (for example filament not present -> disable fs)
    /// @returns true if the user confirmed to skip all warnings
    /// @param skip_level Skips incompatibilities with the provided compatibility level and better
    /// !!! TO BE EXECUTED FROM THE GUI THREAD ONLY
    [[nodiscard]] bool gui_confirm_all_incompatibilities(Response abort_response = Response::Abort, CompatibilityLevel skip_level = CompatibilityLevel::fully_compatible, auto... visitor_args) const {
        // If there is any error, show it first and don't bother with warnings
        const auto highest_severity_failed_check = this->highest_severity_failed_check();
        if (auto &check = highest_severity_failed_check; check.has_value() && check->meta->evaluate_compatibility() >= CompatibilityLevel::fatal_incompatibility) {
            gui_incompatibility_error(*check->meta, abort_response);
            return false;
        }

        return static_cast<const Report *>(this)->visit_failed_checks([this, abort_response, skip_level](const typename Report::FailedCheck &check) -> bool {
            const auto compatibility = check.meta->evaluate_compatibility();
            if (compatibility <= skip_level) {
                return true;
            }

            return static_cast<const Report *>(this)->gui_confirm_incompatibility(check, abort_response);
        },
            visitor_args...);
    }
};

} // namespace buddy::compatibility_checks
