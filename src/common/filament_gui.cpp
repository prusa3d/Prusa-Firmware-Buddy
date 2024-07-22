#include "filament_gui.hpp"

static constexpr IWindowMenuItem::ColorScheme hidden_filament_color_scheme {
    .text = {
        .focused = COLOR_GRAY,
        .unfocused = COLOR_GRAY,
    },
};

void FilamentTypeGUI::setup_menu_item([[maybe_unused]] FilamentType ft, const FilamentTypeParameters &params, IWindowMenuItem &item) {
    item.SetLabel(string_view_utf8::MakeRAM(params.name));
    item.set_color_scheme(ft.is_visible() ? nullptr : &hidden_filament_color_scheme);
    item.SetIconId(std::holds_alternative<UserFilamentType>(ft) ? &img::user_filament_16x16 : nullptr);
}

bool FilamentTypeGUI::validate_user_filament_name(char *name) {
    // Name must not be empty
    if (strlen(name) == 0) {
        return false;
    }

    // Name must not be "---"
    if (strcmp(name, "---") == 0) {
        return false;
    }

    while (char &ch = *name) {
        // Name must consist only of alphanumeric characters (plus some extra)
        if (!isalnum(ch) && !strchr("_-", ch)) {
            return false;
        }

        // All characters go to upper
        ch = static_cast<char>(toupper(ch));
        name++;
    }

    return true;
}
