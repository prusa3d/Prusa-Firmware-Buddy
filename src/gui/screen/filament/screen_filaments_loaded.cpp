#include "screen_filaments_loaded.hpp"
#include "screen_filament_detail.hpp"

#include <utils/string_builder.hpp>
#include <print_utils.hpp>
#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <gui_utils.hpp>

#include <option/has_anfc.h>
#if HAS_ANFC()
    #include <feature/openprinttag/utils.hpp>
    #include <gui/screen/openprinttag/screen_opt_info.hpp>
#endif

namespace {
constexpr int16_t arrow_right_space = 16;
constexpr int16_t loaded_filament_color_swatch_space = 32;
} // namespace

MI_LOADED_FILAMENT::MI_LOADED_FILAMENT(DisplayFormat display_format, uint8_t tool)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no)
    , tool_(VirtualToolIndex::from_raw(tool))
    , display_format_(display_format) {

    should_open_submenu_ = (display_format == DisplayFormat::auto_submenu) && (get_num_of_enabled_tools() > 1);

    StringBuilder info_sb { info_buffer_ };
    string_view_utf8 label;

    if (should_open_submenu_) {
        label = _("Loaded filaments");

    } else {
        filament_type_ = config_store().get_filament_type(tool);

        if (display_format == DisplayFormat::auto_submenu) {
#if HAS_MINI_DISPLAY()
            // Longer text doesn't fit well on the mini display
            label = _("Loaded");
#else
            label = _("Loaded filament");
#endif
        } else {
            label = _("Filament %d").formatted(label_params_, tool_.display_index());
        }

        filament_type_.build_name_with_info(info_sb);
        color_ = filament_type_.parameters().color;

        set_enabled(filament_type_ != FilamentType::none);
        set_is_hidden(!tool_.is_enabled());
    }

    extension_width = arrow_right_space + (color_.has_value() ? loaded_filament_color_swatch_space : 0) + string_view_utf8::MakeRAM(info_sb.str()).computeNumUtf8Chars() * width(IWiInfo::font);
    extension_width = std::min(extension_width, IWiInfo::max_extension_width());
    SetLabel(label);
}

void MI_LOADED_FILAMENT::click(IWindowMenu &) {
    if (should_open_submenu_) {
        Screens::Access()->Open<ScreenLoadedFilaments>();
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

    render_icon_align(extension_rect, &img::arrow_right_10x16, color_back, Align_t::RightCenter(), raster_op);
    extension_rect -= Rect16::Width_t { arrow_right_space };

    if (color_.has_value()) {
        constexpr auto margin = 6;

        draw_color_swatch(
            Rect16::fromLTRB(
                // margin * 2 - add a bit more extra spacing before the text
                extension_rect.Right() - loaded_filament_color_swatch_space + margin * 2,
                extension_rect.Top() + margin,
                extension_rect.Right(),
                extension_rect.Bottom() - margin),

            Align_t::RightCenter(),
            color_,
            color_back);

        extension_rect -= Rect16::Width_t { loaded_filament_color_swatch_space };
    }

    render_text_align(extension_rect, string_view_utf8::MakeRAM(info_buffer_.data()), IWiInfo::font, color_back, IWiInfo::extension_text_color(*this), GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
}

void MI_LOADED_FILAMENT::Loop() {
#if HAS_ANFC()
    if (!should_open_submenu_) {
        SetIconId(buddy::openprinttag::tool_tag_status_icon(tool_));
    }
#endif
}

ScreenLoadedFilaments::ScreenLoadedFilaments()
    : ScreenMenu(_("LOADED FILAMENTS")) {}
