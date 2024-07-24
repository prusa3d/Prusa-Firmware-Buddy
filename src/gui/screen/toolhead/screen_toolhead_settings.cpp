#include "screen_toolhead_settings.hpp"

#include <common/nozzle_diameter.hpp>
#include <ScreenHandler.hpp>
#include <img_resources.hpp>

using namespace screen_toolhead_settings;

static constexpr NumericInputConfig nozzle_diameter_spin_config_with_special = [] {
    NumericInputConfig result = nozzle_diameter_spin_config;
    result.special_value = 0;
    result.special_value_str = N_("-");
    return result;
}();

// * MI_NOZZLE_DIAMETER
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, nozzle_diameter_spin_config_with_special, _("Nozzle Diameter")) {
    update();
}

float MI_NOZZLE_DIAMETER::read_value_impl(ToolheadIndex ix) {
    return config_store().get_nozzle_diameter(ix);
}

void MI_NOZZLE_DIAMETER::store_value_impl(ToolheadIndex ix, float set) {
    config_store().set_nozzle_diameter(ix, set);
}

// * MI_NOZZLE_DIAMETER_HELP
MI_NOZZLE_DIAMETER_HELP::MI_NOZZLE_DIAMETER_HELP()
    : IWindowMenuItem(_("What nozzle diameter do I have?"), &img::question_16x16) {
}

void MI_NOZZLE_DIAMETER_HELP::click(IWindowMenu &) {
    MsgBoxInfo(_("You can determine the nozzle diameter by counting the markings (dots) on the nozzle:\n"
                 "  0.40 mm nozzle: 3 dots\n"
                 "  0.60 mm nozzle: 4 dots\n\n"
                 "For more information, visit prusa.io/nozzle-types"),
        Responses_Ok);
}

#if HAS_HOTEND_TYPE_SUPPORT()
// * MI_HOTEND_TYPE
MI_HOTEND_TYPE::MI_HOTEND_TYPE()
    : WiStoreEnumSwitch(_("Hotend Type"), hotend_type_names, true, hotend_type_supported) {}

// * MI_NOZZLE_SOCK
MI_NOZZLE_SOCK::MI_NOZZLE_SOCK()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().hotend_type.get() == HotendType::stock_with_sock, _("Nextruder Silicone Sock"), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_NOZZLE_SOCK::OnChange([[maybe_unused]] size_t old_index) {
    config_store().hotend_type.set(index ? HotendType::stock_with_sock : HotendType::stock);
}
#endif

// * ScreenToolheadDetail
ScreenToolheadDetail::ScreenToolheadDetail(Toolhead toolhead)
    : ScreenMenu({})
    , toolhead(toolhead) //
{
    if (toolhead == all_toolheads) {
        header.SetText(_("ALL TOOLS"));
    } else {
        header.SetText(_("TOOL %d").formatted(title_params, std::get<ToolheadIndex>(toolhead) + 1));
    }

    menu_set_toolhead(container, toolhead);

    // Do not show certain items until printer setup is done
    if (!config_store().printer_setup_done.get()) {
    }

    // Some options don't make sense for AllToolheads
    if (toolhead == all_toolheads) {
    }
}

// * MI_TOOLHEAD
MI_TOOLHEAD::MI_TOOLHEAD(Toolhead toolhead)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes)
    , toolhead(toolhead) //
{
    if (toolhead == all_toolheads) {
        SetLabel(_("All Tools"));

    } else {
        const ToolheadIndex ix = std::get<ToolheadIndex>(toolhead);
        SetLabel(_("Tool %d").formatted(label_params, ix + 1));
#if HAS_TOOLCHANGER()
        set_is_hidden(!prusa_toolchanger.is_tool_enabled(ix));
#endif
    }
}

void MI_TOOLHEAD::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenToolheadDetail>(toolhead));
}

#if HAS_TOOLCHANGER()

// * ScreenToolheadSettingsList
ScreenToolheadSettingsList::ScreenToolheadSettingsList()
    : ScreenMenu(_("TOOLS SETTINGS")) //
{}

#endif
