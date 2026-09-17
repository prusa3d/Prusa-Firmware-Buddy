/// @file
#include "menu_item_color.hpp"

#include <utils/color555.hpp>
#include <gui_utils.hpp>
#include <gui/dialogs/dialog_menu_virtual.hpp>
#include <MItem_tools.hpp>

namespace {
struct ColorPreset {
    // Use 5551 - we use that in EEPROM,
    // the compressed colors would not match the presets otherwise
    OptionalColor555 color555;
    TranslatableString name;

    constexpr OptionalColor color() const {
        return color555.transform<OptionalColor>(&Color555::to_color);
    }
};

constexpr const std::array color_presets {
    ColorPreset {
        .color555 = std::nullopt,
        .name = N_("None"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(0, 0, 0),
        .name = N_("Black"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(255, 255, 255),
        .name = N_("White"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(128, 128, 128),
        .name = N_("Gray"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(255, 0, 0),
        .name = N_("Red"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(255, 165, 0),
        .name = N_("Orange"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(255, 255, 0),
        .name = N_("Yellow"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(0, 255, 0),
        .name = N_("Green"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(0, 0, 255),
        .name = N_("Blue"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(128, 0, 128),
        .name = N_("Purple"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(255, 192, 203),
        .name = N_("Pink"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(165, 42, 42),
        .name = N_("Brown"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(245, 245, 220),
        .name = N_("Beige"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(64, 224, 208),
        .name = N_("Turquoise"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(255, 215, 0),
        .name = N_("Gold"),
    },
    ColorPreset {
        .color555 = Color555::from_rgb(192, 192, 192),
        .name = N_("Silver"),
    },
};

class DialogMenuItem : public IWindowMenuItem {

public:
    DialogMenuItem(const ColorPreset &preset)
        : color_(preset.color()) {
        SetLabel(preset.name);
        extension_width = MenuItemColorSwatchExtension {}.compute_width();
    }

protected:
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override {
        (void)color_text;

        const MenuItemColorSwatchExtension ext {
            .color = color_,
        };
        ext.draw(*this, extension_rect, color_back, raster_op);
    }

    void click(IWindowMenu &menu) override {
        // Use the native dialog_menu_virtual handling system
        dialog_menu_virtual::MenuItem::handle_click(*this, menu);
    }

private:
    const OptionalColor color_;
};

}; // namespace

MenuItemColor::MenuItemColor(const string_view_utf8 &label)
    : IWindowMenuItem(label) {
    extension_width = MenuItemColorSwatchExtension { .show_arrow = true }.compute_width();
}

void MenuItemColor::set_value(OptionalColor new_value) {
    if (new_value == value_) {
        return;
    }

    value_ = new_value;
    InValidateExtension();
}

void MenuItemColor::set_is_editable(bool set) {
    if (is_editable_ == set) {
        return;
    }

    is_editable_ = set;
    InValidateExtension();
}

void MenuItemColor::printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const {
    (void)color_text;

    const MenuItemColorSwatchExtension ext {
        .color = value_,
        .show_arrow = is_editable_ && IsEnabled(),
    };
    ext.draw(*this, extension_rect, color_back, raster_op);
}

void MenuItemColor::click(IWindowMenu &menu) {
    if (is_editable_) {
        const auto prev_focus = menu.focused_item_index();

        if (const auto color_opt = color_prompt_dialog(GetLabel(), value_)) {
            set_value(*color_opt);
        }

        // Opening a dialog with a menu screws up focus for the current menu - we need to restore it
        menu.move_focus_to_index(prev_focus);
    }
}

std::optional<OptionalColor> MenuItemColor::color_prompt_dialog(const string_view_utf8 &title, OptionalColor default_color) {
    const auto c5 = default_color.transform<OptionalColor555>(Color555::from_color);
    const auto preset_it = std::ranges::find_if(
        color_presets,
        [c5](const ColorPreset &preset) { return preset.color555 == c5; });

    const bool is_preset_color = preset_it != color_presets.end();

    const dialog_menu_virtual::Config config {
        .title = title,
        // + MI_RETURN
        .item_count = color_presets.size() + 1,
        // + 1 -> MI_RETURN
        .focused_item = is_preset_color ? (1 + preset_it - color_presets.begin()) : -1,
        .setup_f = [](int index, dialog_menu_virtual::ItemVariant &variant) {
            if (index == 0) {
                variant.emplace<MI_RETURN>();
            } else {
                variant.emplace<DialogMenuItem>(color_presets[index - 1]);
            }
        },
    };
    const auto result = dialog_menu_virtual::execute(config);
    if (!result || result == 0) {
        return std::nullopt;
    }

    // - MI_RETURN
    return color_presets[*result - 1].color();
}
