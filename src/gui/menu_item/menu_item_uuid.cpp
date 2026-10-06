/// @file
#include "menu_item_uuid.hpp"

#include <WindowMenuInfo.hpp>
#include <gui/dialogs/dialog_text_input.hpp>
#include <window_msgbox.hpp>

namespace {
constexpr Font font = IWiInfo::font;
constexpr size_t peek_chars = 9;
constexpr TranslatableString nullopt_str = N_("NONE");
} // namespace

MenuItemUUID::MenuItemUUID(const string_view_utf8 &label, const Value &value)
    : IWindowMenuItem(label)
    , value_(value) {
    extension_width = std::max<int>(peek_chars, string_view_utf8(nullopt_str).computeNumUtf8Chars()) * width(font);
}

void MenuItemUUID::set_value(const Value &set) {
    if (value_ == set) {
        return;
    }

    value_ = set;
    InValidateExtension();
}

void MenuItemUUID::printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const {
    (void)color_text, (void)raster_op;

    string_view_utf8 text = nullopt_str;
    std::array<char, UUID::string_length> uuid_buf;

    if (value_.has_value()) {
        std::ignore = value_->to_string(uuid_buf);
        uuid_buf[peek_chars] = '\0';
        text = string_view_utf8::MakeRAM(uuid_buf.data());
    }

    render_text_align(extension_rect, text, font, color_back, IWiInfo::extension_text_color(*this), GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
}

void MenuItemUUID::click(IWindowMenu &) {
    std::array<char, UUID::string_length + 1> str_buf;
    str_buf[0] = '\0';
    if (value_.has_value()) {
        str_buf[value_->to_string(str_buf).length()] = '\0';
    }

    while (true) {
        if (!DialogTextInput::exec(GetLabel(), str_buf)) {
            return;
        }

        const std::string_view str { str_buf.data() };

        if (str.empty()) {
            set_value(std::nullopt);
            return;
        }

        const auto uuid = UUID::from_string(str);
        if (!uuid.has_value()) {
            MsgBoxError(_("Entered value is not a valid UUID"), Responses_Ok);
            continue;
        }

        set_value(Value { *uuid, Value::NulloptCheck::no });
        return;
    }
}
