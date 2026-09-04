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
constexpr auto arrow_icon_margin = 6;
constexpr auto color_swatch_section_w = 32;
} // namespace

MI_LOADED_FILAMENT::MI_LOADED_FILAMENT(DisplayFormat display_format, uint8_t tool)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no)
    , tool_(VirtualToolIndex::from_raw(tool))
    , display_format_(display_format) {

    extension_width = img::arrow_right_10x16.w + arrow_icon_margin + color_swatch_section_w;
    should_open_submenu_ = (display_format == DisplayFormat::auto_submenu) && (get_num_of_enabled_tools() > 1);

    if (should_open_submenu_) {
        SetLabel(_("Loaded filaments"));

    } else {
        filament_type_ = config_store().get_filament_type(tool);

        StringBuilder sb(label_buffer_);
        if (display_format == DisplayFormat::auto_submenu) {
#if HAS_MINI_DISPLAY()
            // Longer text doesn't fit well on the mini display
            sb.append_string_view(_("Loaded"));
#else
            sb.append_string_view(_("Loaded filament"));
#endif
        } else {
            sb.append_string_view(_("Filament"));
            sb.append_printf(" %d", tool_.display_index());
        }

        sb.append_string(": ");
        filament_type_.build_name_with_info(sb);
        color_ = filament_type_.parameters().color;

        SetLabel(string_view_utf8::MakeRAM(label_buffer_.data()));
        set_enabled(filament_type_ != FilamentType::none);
        set_is_hidden(!tool_.is_enabled());
    }
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

    // Some margin
    extension_rect -= Rect16::Width_t { img::arrow_right_10x16.w + arrow_icon_margin };

    if (color_.has_value()) {
        constexpr auto margin = 6;
        draw_color_swatch(
            Rect16::CutPadding(extension_rect, padding_ui8_t { .top = margin, .bottom = margin }),
            Align_t::RightCenter(),
            color_,
            color_back);
    }
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
