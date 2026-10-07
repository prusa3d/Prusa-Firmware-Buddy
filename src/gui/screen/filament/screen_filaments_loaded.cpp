#include "screen_filaments_loaded.hpp"
#include "screen_filament_detail.hpp"

#include <utils/string_builder.hpp>
#include <print_utils.hpp>
#include <ScreenHandler.hpp>
#include <WindowMenuItems.hpp>
#include <img_resources.hpp>
#include <gui_utils.hpp>
#include <gui/screen/screen_menu_virtual.hpp>

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <feature/openprinttag/utils.hpp>
    #include <gui/screen/openprinttag/screen_opt_info.hpp>
#endif

namespace {
constexpr int16_t arrow_right_space = 16;
constexpr int16_t loaded_filament_color_swatch_space = 32;

constexpr screen_menu_virtual::Configuration loaded_filaments_menu_config {
    .item_count = [] -> int { return VirtualToolIndex::enabled_range_size() + 1; }, // + MI_RETURN
    .item_constructor = [](WindowMenuVirtual::ItemVariant &variant, int index) {
        if (index == 0) {
            variant.emplace<MI_RETURN>();
        } else {
            variant.emplace<MI_LOADED_FILAMENT>(MI_LOADED_FILAMENT::DisplayFormat::slot_number, VirtualToolIndex::from_raw(index - 1));
        } },
    .title = N_("LOADED FILAMENTS"),
};
} // namespace

MI_LOADED_FILAMENT::MI_LOADED_FILAMENT(DisplayFormat display_format, VirtualToolIndex tool)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no)
    , tool_(tool)
    , display_format_(display_format) {

    should_open_submenu_ = (display_format == DisplayFormat::auto_submenu) && (get_num_of_enabled_tools() > 1);
    extension_.show_arrow = true;
    extension_.show_null_color = false;

    string_view_utf8 label;

    if (should_open_submenu_) {
        label = _("Loaded Filaments");

    } else {
        filament_type_ = config_store().get_filament_type(tool);

        if (display_format == DisplayFormat::auto_submenu) {
#if HAS_MINI_DISPLAY()
            // Longer text doesn't fit well on the mini display
            label = _("Loaded");
#else
            label = _("Loaded Filament");
#endif
        } else {
            label = _("Filament %d").formatted(label_params_, tool_.display_index());
        }

        StringBuilder info_sb { info_buffer_ };
        filament_type_.build_name_with_info(info_sb);

        extension_.text = string_view_utf8::MakeRAM(info_buffer_.data());
        extension_.color = filament_type_.parameters().color;

        set_enabled(filament_type_ != FilamentType::none && tool_.is_enabled());
    }

    SetLabel(label);
    extension_width = extension_.compute_width();
}

void MI_LOADED_FILAMENT::click(IWindowMenu &) {
    if (should_open_submenu_) {
        Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenMenuVirtual>(&loaded_filaments_menu_config));
    } else {
#if HAS_ANFC()
        Screens::Access()->Open(buddy::openprinttag::screen_opt_info_loaded_creator(tool_));
#else
        Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenFilamentDetail>(ScreenFilamentDetail::LoadedFilamentMode { .tool = tool_ }));
#endif
    }
}

void MI_LOADED_FILAMENT::printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const {
    (void)color_text;
    extension_.draw(*this, extension_rect, color_back, raster_op);
}

void MI_LOADED_FILAMENT::Loop() {
#if HAS_ANFC()
    if (!should_open_submenu_) {
        SetIconId(buddy::openprinttag::tool_tag_status_icon(tool_));
    }
#endif
}
