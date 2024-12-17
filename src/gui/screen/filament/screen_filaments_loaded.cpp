#include "screen_filaments_loaded.hpp"
#include "screen_filament_detail.hpp"

#include <str_utils.hpp>
#include <print_utils.hpp>
#include <ScreenHandler.hpp>

MI_LOADED_FILAMENT::MI_LOADED_FILAMENT(DisplayFormat display_format, uint8_t tool)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes)
    , display_format_(display_format)
    , tool_(tool) //
{
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
            sb.append_printf(" %d", tool + 1);
        }

        sb.append_string(": ");
        filament_type_.build_name_with_info(sb);

        SetLabel(string_view_utf8::MakeRAM(label_buffer_.data()));
        set_enabled(filament_type_ != FilamentType::none);
        set_is_hidden(!is_tool_enabled(tool));
    }
}

void MI_LOADED_FILAMENT::click(IWindowMenu &) {
    if (should_open_submenu_) {
        Screens::Access()->Open<ScreenLoadedFilaments>();
    } else {
        Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenFilamentDetail>(EncodedFilamentType(filament_type_)));
    }
}

ScreenLoadedFilaments::ScreenLoadedFilaments()
    : ScreenMenu(_("LOADED FILAMENTS")) {}
