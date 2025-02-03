#include "filament_gui.hpp"

static constexpr IWindowMenuItem::ColorScheme hidden_filament_color_scheme {
    .text = {
        .focused = COLOR_GRAY,
        .unfocused = COLOR_GRAY,
    },
};

void FilamentTypeGUI::setup_menu_item([[maybe_unused]] FilamentType ft, const FilamentTypeParameters::Name &name_buf, IWindowMenuItem &item) {
    item.SetLabel(string_view_utf8::MakeRAM(name_buf.data()));
    item.set_color_scheme(ft.is_visible() ? nullptr : &hidden_filament_color_scheme);
    item.SetIconId(std::holds_alternative<UserFilamentType>(ft) ? &img::user_filament_16x16 : nullptr);
}
