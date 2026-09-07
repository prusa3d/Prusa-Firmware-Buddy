#include "screen_opt_filament_detail.hpp"
#include "screen_opt_filament_detail_private.hpp"

#include <feature/openprinttag/requests_read_multi.hpp>
#include <feature/openprinttag/data_utils.hpp>
#include <gui/screen/openprinttag/opt_request_wizard.hpp>

namespace buddy::openprinttag {

ScreenFactory::Creator screen_openprinttag_filament_detail_creator(ToolTag tag) {
    const ScreenOPTFilamentDetail::InfoParams open_args {
        .tag = tag,
    };
    return ScreenFactory::ScreenWithArg<ScreenOPTFilamentDetail>(open_args);
}

ScreenFactory::Creator screen_openprinttag_preheat_mode_creator(ToolTag tag, PreheatMode mode) {
    const ScreenOPTFilamentDetail::PreheatModeParams open_args {
        .uid_hash = tag.uid_hash(),
        .tool = tag.tool(),
        .mode = mode,
    };
    return ScreenFactory::ScreenWithArg<ScreenOPTFilamentDetail>(open_args);
}

ScreenOPTFilamentDetail::ScreenOPTFilamentDetail(InfoParams params)
    : ScreenFilamentDetail(N_("OPENPRINTTAG INFO"))
    , tag_(params.tag) {

    // First scan needs to be delayed (cannot be in the constructor)
    scan_pending_ = true;

    // Hide the "openprinttag linked" item, it would always be true and disabled, pointless/confusing
    Item<screen_filament_detail::MI_FILAMENT_ASSIGNED_OPENPRINTTAG>().set_is_hidden(true);
}

ScreenOPTFilamentDetail::ScreenOPTFilamentDetail(PreheatModeParams params)
    : ScreenFilamentDetail(N_("SCAN OPENPRINTTAG"))
    , tag_(ToolTag(params.tool, params.uid_hash)) {

    setup_preheat_mode_confirm({
        .tool = tag_.tool(),
        .mode = params.mode,
    });

    // First scan needs to be delayed (cannot be in the constructor)
    scan_pending_ = true;

    preheat_mode_ = true;
}

bool ScreenOPTFilamentDetail::scan() {
    using MultiRequest = MultiReadFieldRequest<FilamentParametersInfo::Requirements {}>;

    MultiRequest req { tag_ };

    if (!multirequest_with_troubleshooting(req)) {
        close_screen();
        return false;
    }

    FilamentParametersInfo params { req };

    FilamentType filament_type = preheat_mode_ ? FilamentType { FilamentType::pending_adhoc } : FilamentType { FilamentType::none };
    setup(filament_type, params.parameters);

    static constexpr IWindowMenuItem::ColorScheme missing_item_color_scheme {
        .text = {
            .focused = COLOR_BRAND,
            .unfocused = COLOR_BRAND,
        },
    };

    // Indicate what parameters were missing on the NFC tag
    stdext::visit_tuple(container.menu_items, [&]<typename T>(T &item) {
        static constexpr bool is_utility_button = std::is_same_v<T, MI_RETURN> || std::is_same_v<T, screen_filament_detail::MI_CONFIRM>;

        if constexpr (is_utility_button || std::is_same_v<T, screen_filament_detail::MI_FILAMENT_VISIBLE>) {
            // pass
        } else {
            item.set_color_scheme(params.is_missing<T::parameter_ptr>() ? &missing_item_color_scheme : nullptr);
        }
    });

    if (!params.data_safe_to_use) {
        MsgBoxWarning(_("Some of the parameters on the OpenPrintTag were missing or invalid. Please revise the highlighted fields."), Responses_Ok);

    } else if (preheat_mode_) {
        menu.menu.move_focus_to_index(menu.menu.GetIndex(Item<screen_filament_detail::MI_CONFIRM>()));
    }

    return true;
}

void ScreenOPTFilamentDetail::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::LOOP:
        if (scan_pending_) {
            scan_pending_ = false;
            scan();
        }
        break;

    default:
        break;
    }

    ScreenFilamentDetail::screenEvent(sender, event, param);
}
} // namespace buddy::openprinttag
