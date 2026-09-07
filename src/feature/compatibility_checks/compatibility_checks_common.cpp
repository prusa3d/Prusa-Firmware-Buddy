/// @file
#include "compatibility_checks_common.hpp"

#include <utils/overloaded_visitor.hpp>
#include <config_store/store_instance.hpp>
#include <window_msgbox.hpp>
#include <img_resources.hpp>
#include <i_window_menu_item.hpp>

namespace buddy::compatibility_checks {

namespace {
    CompatibilityLevel to_compatibility_level(HWCheckSeverity v) {
        switch (v) {

        case HWCheckSeverity::Abort:
            return CompatibilityLevel::fatal_incompatibility;

        case HWCheckSeverity::Warning:
            return CompatibilityLevel::needs_user_approval;

        case HWCheckSeverity::Ignore:
            return CompatibilityLevel::fully_compatible;
        }

        bsod_unreachable();
    }

    constexpr IWindowMenuItemColorScheme menu_item_scheme_needs_user_approval {
        .text {
            .focused = COLOR_ORANGE,
            .unfocused = COLOR_ORANGE,
        },
    };
    constexpr IWindowMenuItemColorScheme menu_item_scheme_fatal_incompatibility {
        .text {
            .focused = COLOR_RED,
            .unfocused = COLOR_RED,
        },
    };

} // namespace

constinit const EnumArray<CompatibilityLevel, const img::Resource *, std::to_underlying(CompatibilityLevel::_last) + 1> compatibility_level_icons {
    { CompatibilityLevel::fully_compatible, nullptr },
    { CompatibilityLevel::compatible_with_reminder, &img::info_16x16 },
    { CompatibilityLevel::needs_user_approval, &img::warning_16x16 },
    { CompatibilityLevel::fatal_incompatibility, &img::nok_color_16x16 },
};

constinit const EnumArray<CompatibilityLevel, const IWindowMenuItem::ColorScheme *, std::to_underlying(CompatibilityLevel::_last) + 1> compatibility_level_menu_item_color_schemes {
    { CompatibilityLevel::fully_compatible, &IWindowMenuItem::color_scheme_default },
    { CompatibilityLevel::compatible_with_reminder, &IWindowMenuItem::color_scheme_default },
    { CompatibilityLevel::needs_user_approval, &menu_item_scheme_needs_user_approval },
    { CompatibilityLevel::fatal_incompatibility, &menu_item_scheme_fatal_incompatibility },
};

CompatibilityLevel CheckMetadata::evaluate_compatibility() const {
    return match(
        severity, //
        [](CompatibilityLevel v) -> CompatibilityLevel { return v; }, //
        [](HWCheckSeverity v) -> CompatibilityLevel { return to_compatibility_level(v); }, //
        [](HWCheckType type) -> CompatibilityLevel { return to_compatibility_level(config_store().visit_hw_check(type, [](auto &t) { return t.get(); })); } //
    );
}

void gui_incompatibility_error(const CheckMetadata &check, Response abort_response) {
    MsgBoxError(_(check.description), { abort_response });
}

[[nodiscard]] bool gui_confirm_incompatibility_default(const CheckMetadata &check, Response abort_response) {
    switch (check.evaluate_compatibility()) {

    case CompatibilityLevel::fully_compatible:
        return true;

    case CompatibilityLevel::fatal_incompatibility:
        gui_incompatibility_error(check, abort_response);
        return false;

    case CompatibilityLevel::needs_user_approval:
        return MsgBoxWarning(_(check.description), { abort_response, Response::Ignore }) == Response::Ignore;

    case CompatibilityLevel::compatible_with_reminder:
        return MsgBoxInfo(_(check.description), { abort_response, Response::Ok }) == Response::Ok;
    }
    bsod_unreachable();
}
} // namespace buddy::compatibility_checks
