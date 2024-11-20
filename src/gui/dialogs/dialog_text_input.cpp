#include "dialog_text_input.hpp"
#include "dialog_text_input_layout.in.cpp"

#include <ScreenHandler.hpp>
#include <sound.hpp>
#include <img_resources.hpp>
#include "timing.h"

using namespace dialog_text_input;

namespace {
constexpr color_scheme button_text_color_scheme = {
    .normal = COLOR_WHITE,
    .focused = COLOR_VERY_DARK_GRAY,
    .shadowed = COLOR_VERY_DARK_GRAY,
    .focused_and_shadowed = COLOR_GRAY,
};

constexpr color_scheme button_background_color_scheme = {
    .normal = COLOR_VERY_DARK_GRAY,
    .focused = COLOR_WHITE,
    .shadowed = COLOR_BLACK,
    .focused_and_shadowed = COLOR_DARK_GRAY,
};

constexpr color_scheme special_button_background_color_scheme = {
    .normal = COLOR_DARK_GRAY,
    .focused = COLOR_VERY_LIGHT_GRAY,
    .shadowed = COLOR_BLACK,
    .focused_and_shadowed = COLOR_DARK_GRAY,
};

constexpr Rect16 buttons_rect = Rect16::fromLTRB(buttons_rect_margin, GuiDefaults::HeaderHeight + 72, GuiDefaults::ScreenWidth - buttons_rect_margin, GuiDefaults::ScreenHeight - buttons_rect_margin);

constexpr size_ui16_t button_grid_item_size = { buttons_rect.Width() / button_cols, buttons_rect.Height() / button_rows };
constexpr size_ui16_t button_size = { button_grid_item_size.w - button_padding * 2, button_grid_item_size.h - button_padding * 2 };

constexpr EnumArray<SpecialButton, std::variant<const char *, const img::Resource *>, SpecialButton::_cnt> special_button_labels {
#if HAS_LARGE_DISPLAY()
    { SpecialButton::ok, &img::ok_60x60 },
        { SpecialButton::cancel, &img::cancel_60x60 },
        { SpecialButton::clear, &img::clear_60x60 },
        { SpecialButton::backspace, &img::backspace_60x60 },
        { SpecialButton::space, &img::space_60x60 },
#else
    { SpecialButton::ok, &img::ok_44x33 },
        { SpecialButton::cancel, &img::cancel_44x33 },
        { SpecialButton::clear, &img::clear_44x33 },
        { SpecialButton::backspace, &img::backspace_44x33 },
        { SpecialButton::space, &img::space_44x33 },
#endif

        { SpecialButton::uppercase, "ABC" },
        { SpecialButton::lowercase, "abc" },
        { SpecialButton::symbols, "!&$" },
        { SpecialButton::numbers, "123" },
};

} // namespace

DialogTextInput::DialogTextInput(const string_view_utf8 &prompt, std::span<char> buffer)
    : buffer_(buffer) {
    ui.txt_prompt.SetText(prompt);
    setup_ui();
    set_keyboard_layout(layout_text_lowercase);

    update_result();
    update_ui_enabled();

    CaptureNormalWindow(*this);

    // We need to set focus to something, otherwise touch wouldn't work
    ui.btn_matrix[0].SetFocus();
}

bool DialogTextInput::exec(const string_view_utf8 &prompt, std::span<char> buffer) {
    DialogTextInput dlg(prompt, buffer);
    Screens::Access()->gui_loop_until_dialog_closed();
    return !dlg.cancelled_;
}

void DialogTextInput::screenEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *const param) {
    switch (event) {

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        // Do not accept swipe gesture over the buttons, to prevent accidentaly going back
        if (get_child_by_touch_point(event_conversion_union { .pvoid = param }.point)) {
            return;
        }

        Sound_Play(eSOUND_TYPE::ButtonEcho);
        cancelled_ = true;
        Screens::Access()->Close();
        return;

    case GUI_event_t::LOOP:
        // If the user has not edited the char for some time, flush it
        if (edit_char_buffer_[0] && ticks_diff(ticks_ms(), last_char_edit_ms_) > 1000) {
            flush_edit_char();
        }
        break;

    default:
        break;
    }

    IDialog::screenEvent(sender, event, param);
}

void DialogTextInput::setup_ui() {
    int16_t y = GuiDefaults::HeaderHeight + 8;

    // Setup prompt text
    {
        constexpr uint16_t h = 24;

        auto &wnd = ui.txt_prompt;
        wnd.SetRect(Rect16::fromLTRB(buttons_rect.Left() + button_padding, y, buttons_rect.Right() - button_padding, y + h));
        wnd.SetTextColor(COLOR_WHITE);
        wnd.set_font(Font::small);
        wnd.SetAlignment(Align_t::Right());
        wnd.set_enabled(false);

        RegisterSubWin(wnd);
        y += h;
    }

    // Setup result text
    {
        constexpr uint16_t h = 32;
        constexpr uint16_t r = buttons_rect.Right() - 18;
        const int16_t b = y + h;

        {
            auto &wnd = ui.txt_result;
            wnd.SetRect(Rect16::fromLTRB(buttons_rect.Left() + button_padding, y, r, b));
            wnd.set_font(Font::big);
            wnd.SetTextColor(COLOR_ORANGE);
            wnd.SetAlignment(Align_t::RightCenter());
            wnd.set_enabled(false);

            RegisterSubWin(wnd);
        }

        // Setup edit char text
        {
            auto &wnd = ui.txt_edit_char;
            wnd.SetRect(Rect16::fromLTRB(r, y, buttons_rect.Right() - button_padding, b));
            wnd.set_font(Font::big);
            wnd.SetText(string_view_utf8::MakeRAM(edit_char_buffer_.data()));
            wnd.SetTextColor(COLOR_YELLOW);
            wnd.SetAlignment(Align_t::LeftCenter());
            wnd.set_enabled(false);

            RegisterSubWin(wnd);
        }
    }

    // Setup buttons matrix
    {
        const auto button_callback = [this](window_t &button) { this->button_callback(button); };

        for (int i = 0; i < button_count; i++) {
            const auto col = i % button_cols;
            const auto row = i / button_cols;
            auto &btn = ui.btn_matrix[i];

            btn.callback = button_callback;

            btn.SetRect(Rect16(buttons_rect.Left() + col * button_grid_item_size.w + button_padding, buttons_rect.Top() + row * button_grid_item_size.h + button_padding, button_size.w, button_size.h));
            btn.SetRoundCorners();
            btn.SetAlignment(Align_t::Center());
            btn.set_font(Font::big);
            btn.SetTextColor(button_text_color_scheme);

            RegisterSubWin(btn);
        }
    }

    Invalidate();
}

void DialogTextInput::update_ui_enabled() {
    const bool is_buffer_full = (strlen(buffer_.data()) >= buffer_.size() - 1);

    for (int i = 0; i < button_count; i++) {
        auto &btn = ui.btn_matrix[i];
        const ButtonRec &rec = (*current_layout_)[i / button_cols][i % button_cols];

        // We're using shadowing instead of disabling because it would screw up focusing & knob control
        btn.set_shadow(is_buffer_full && rec.is_character_emitting());
    }
}

void DialogTextInput::set_keyboard_layout(const ButtonsLayout &layout) {
    current_layout_ = &layout;

    for (int i = 0; i < button_count; i++) {
        auto &btn = ui.btn_matrix[i];
        const ButtonRec &rec = layout[i / button_cols][i % button_cols];
        const bool is_special = rec.is_special();

        btn.set_visible(rec != ButtonRec());
        btn.SetBackColor(is_special ? special_button_background_color_scheme : button_background_color_scheme);

        if (is_special) {
            const auto &label = special_button_labels[rec.to_special_button()];
            if (std::holds_alternative<const img::Resource *>(label)) {
                btn.set_icon(std::get<const img::Resource *>(label));
            } else {
                btn.SetText(string_view_utf8::MakeCPUFLASH(std::get<const char *>(label)));
            }
        } else {
            btn.SetText(string_view_utf8::MakeCPUFLASH(rec.data()));
        }
    }

    update_ui_enabled();
}

void dialog_text_input::DialogTextInput::button_callback(window_t &button) {
    // We're using shadowing instead of disabling because it would screw up focusing & knob control
    if (button.IsShadowed()) {
        return;
    }

    // Determine which button from the array the user pressed
    int button_ix = reinterpret_cast<window_text_button_t *>(&button) - ui.btn_matrix;
    assert(button_ix >= 0 && button_ix < button_count);

    const ButtonRec &rec = (*current_layout_)[button_ix / button_cols][button_ix % button_cols];
    char &edit_char = edit_char_buffer_[0];

    switch (rec.to_special_button()) {

    case SpecialButton::ok:
        flush_edit_char();
        cancelled_ = false;
        Screens::Access()->Close();
        return;

    case SpecialButton::cancel:
        cancelled_ = true;
        Screens::Access()->Close();
        return;

    case SpecialButton::clear:
        flush_edit_char();
        buffer_[0] = '\0';
        update_result();
        break;

    case SpecialButton::backspace:
        // There is something in edit char buffer -> clear the buffer
        if (edit_char_buffer_[0]) {
            edit_char_buffer_[0] = '\0';
            ui.txt_edit_char.Invalidate();
        }

        // Otherwise remove one character from the text buffer
        else if (const auto len = strlen(buffer_.data()); len > 0) {
            buffer_[len - 1] = '\0';
            update_result();
        }
        break;

    case SpecialButton::space:
        flush_edit_char();
        edit_char_buffer_[0] = ' ';
        flush_edit_char();
        break;

    case SpecialButton::lowercase:
        set_keyboard_layout(layout_text_lowercase);
        break;

    case SpecialButton::uppercase:
        set_keyboard_layout(layout_text_uppercase);
        break;

    case SpecialButton::symbols:
        set_keyboard_layout(layout_symbols);
        break;

    case SpecialButton::numbers:
#if HAS_NUMBERS_LAYOUT()
        set_keyboard_layout(layout_numbers);
#endif
        break;

    // Not a special button - appending procedure
    case SpecialButton::_cnt: {
        last_char_edit_ms_ = ticks_ms();

        // The button has only one char assigned -> no alternating, straight out print the character
        if (rec[1] == '\0') {
            flush_edit_char();
            edit_char = rec[0];
            flush_edit_char();
            break;
        }

        const auto found = std::find(rec.begin(), rec.end(), edit_char_buffer_[0]);

        // Buffer is not on the current key -> flush and set to first
        if (edit_char == '\0' || found == rec.end()) {
            flush_edit_char();
            edit_char = rec[0];
        }

        // Otherwise cycle
        else {
            edit_char = rec[found - rec.begin() + 1];

            // All recs are zero-terminated, so if we hit \0, wrap and start from the first
            if (edit_char == '\0') {
                edit_char = rec[0];
            }
        }

        ui.txt_edit_char.Invalidate();
        break;
    }
    }
}

void dialog_text_input::DialogTextInput::flush_edit_char() {
    const char ch = edit_char_buffer_[0];
    edit_char_buffer_[0] = '\0';
    ui.txt_edit_char.Invalidate();

    const size_t pos = strlen(buffer_.data());

    if (ch == '\0' || pos >= buffer_.size() - 1) {
        return;
    }

    buffer_[pos] = ch;
    buffer_[pos + 1] = '\0';

    update_result();
}

void dialog_text_input::DialogTextInput::update_result() {
    // Only show the amount of characters that fit on the screen, otherwise we end up seeing the left part and not the right one
    const auto max_visible_characters = ui.txt_result.Width() / resource_font(ui.txt_result.get_font())->w;

    const size_t text_len = strlen(buffer_.data());
    ui.txt_result.SetText(string_view_utf8::MakeRAM(buffer_.data() + text_len - std::min<size_t>(text_len, max_visible_characters)));
    ui.txt_result.Invalidate();

    update_ui_enabled();
}
