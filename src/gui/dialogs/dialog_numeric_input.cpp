#include "dialog_numeric_input.hpp"

#include <sound.hpp>
#include <ScreenHandler.hpp>
#include <scope_guard.hpp>
#include <img_resources.hpp>
#include "logging/log.hpp"

LOG_COMPONENT_REF(GUI);

namespace {

using ButtonType = DialogNumericInput::ButtonType;

constexpr std::array<ButtonType, DialogNumericInput::matrix_count> matrix_layout {
    ButtonType::n7,
    ButtonType::n8,
    ButtonType::n9,
    ButtonType::n4,
    ButtonType::n5,
    ButtonType::n6,
    ButtonType::n1,
    ButtonType::n2,
    ButtonType::n3,
    ButtonType::minus,
    ButtonType::n0,
    ButtonType::decimal_point,
};

constexpr std::array bottom_line_layout {
    ButtonType::ok,
    ButtonType::none,
    ButtonType::clear,
    ButtonType::backspace,
};

constexpr std::array<const char *, 10> number_texts {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
};

} // namespace

DialogNumericInput::Result DialogNumericInput::exec(const string_view_utf8 &prompt, double initial_value, const Config &config) {
    DialogNumericInput dlg(prompt, initial_value, config);
    Screens::Access()->gui_loop_until_dialog_closed();
    return dlg.result();
}

DialogNumericInput::Result DialogNumericInput::result() const {
    if (cancelled_) {
        return std::nullopt;
    }

    if (is_special_value_) {
        return config_.special_value;
    }

    return result_accum_.to_float();
}

DialogNumericInput::DialogNumericInput(const string_view_utf8 &prompt, double initial_value, const Config &config)
    : prompt_(prompt)
    , config_(config) {

    // Set up UI
    {
        constexpr auto button_grid_size = 68;
        constexpr auto button_spacing = 6;
        constexpr auto button_size = button_grid_size - button_spacing;

        constexpr auto matrix_x = GuiDefaults::ScreenWidth - button_grid_size * matrix_cols - 8;
        constexpr auto matrix_y = GuiDefaults::HeaderHeight + 8;

        static constexpr color_scheme button_text_color_scheme = {
            .normal = COLOR_WHITE,
            .focused = COLOR_VERY_DARK_GRAY,
            .shadowed = COLOR_VERY_DARK_GRAY,
            .focused_and_shadowed = COLOR_GRAY,
        };
        static constexpr color_scheme button_background_color_scheme = {
            .normal = COLOR_VERY_DARK_GRAY,
            .focused = COLOR_WHITE,
            .shadowed = COLOR_BLACK,
            .focused_and_shadowed = COLOR_DARK_GRAY,
        };

        static constexpr auto setup_button_common = [](auto *btn) {
            btn->SetBackColor(button_background_color_scheme);
            btn->SetAlignment(Align_t::Center());
            btn->SetRoundCorners();
        };

        static constexpr auto setup_text_button_common = [](window_text_button_t *btn) {
            setup_button_common(btn);
            btn->set_font(Font::large);
            btn->SetTextColor(button_text_color_scheme);
        };

        // Set up number buttons
        {
            const ButtonCallback callback = [this](window_t &button) { this->number_button_callback(button);; };

            for (int i = 0; i < 10; i++) {
                auto *btn = &ui.btn_number[i];
                setup_text_button_common(btn);
                btn->SetText(string_view_utf8::MakeCPUFLASH(number_texts[i]));
                btn->callback = callback;
            }
        }

        // Set up decimal point button
        {
            auto *btn = &ui.btn_decimal;
            setup_text_button_common(btn);
            btn->SetText(string_view_utf8::MakeCPUFLASH("."));
            btn->callback = [this](window_t &button) { decimal_button_callback(button); };
        }

        // Set up negative button
        {
            auto *btn = &ui.btn_minus;
            setup_button_common(btn);
            btn->SetRes(&img::plusminus_60x60);
            btn->SetAction([this](window_t &button) { minus_button_callback(button); });
        }

        // Set up clear button
        {
            auto *btn = &ui.btn_clear;
            setup_button_common(btn);
            btn->SetRes(&img::clear_60x60);
            btn->SetAction([this](window_t &button) { clear_button_callback(button); });
        }

        // Set up backspace button
        {
            auto *btn = &ui.btn_backspace;
            setup_button_common(btn);
            btn->SetRes(&img::backspace_60x60);
            btn->SetAction([this](window_t &button) { backspace_button_callback(button); });
        }

        // Set up ok button
        {
            auto *btn = &ui.btn_ok;
            setup_button_common(btn);
            btn->SetRes(&img::ok_60x60);
            btn->SetAction([this](window_t &button) {  if(!button.IsShadowed()){close(true); } });
        }

        // Set up cancel button
        {
            static constexpr color_scheme scheme = {
                .normal = COLOR_BLACK,
                .focused = COLOR_WHITE,
                .shadowed = COLOR_BLACK,
                .focused_and_shadowed = COLOR_DARK_GRAY,
            };

            auto *btn = &ui.btn_cancel;
            btn->SetBackColor(scheme);
            btn->SetRes(&img::folder_up_16x16);
            btn->SetRect(Rect16::fromLTWH(8, matrix_y + 4, 32, 32));
            btn->SetAlignment(Align_t::Center());
            btn->SetRoundCorners();
            btn->SetAction([this](window_t &button) { if(!button.IsShadowed()){close(false); } });
        }

        // Setup button matrix (mainly positioning)
        {
            for (int i = 0; i < matrix_count; i++) {
                const auto x = i % matrix_cols;
                const auto y = i / matrix_cols;

                auto *btn = ui.button_window(matrix_layout[i]);
                if (!btn) {
                    continue;
                }

                btn->SetRect(Rect16(matrix_x + button_grid_size * x, matrix_y + button_grid_size * y, button_size, button_size));
                RegisterSubWin(*btn);
            }
        }

        // Set up bottom line
        {
            int x = 8;

            for (ButtonType button_type : bottom_line_layout) {
                auto *btn = ui.button_window(button_type);
                if (!btn) {
                    x += 20;
                    continue;
                }

                btn->SetRect(Rect16(x, matrix_y + button_grid_size * (matrix_rows - 1), button_size, button_size));
                RegisterSubWin(*btn);

                x += button_grid_size;
            }
        }

        {
            constexpr int16_t x = 16;
            constexpr int16_t frame_w = matrix_x - 24 - x;
            int16_t y = 55;

            constexpr int16_t result_h = 56;
            const int16_t unit_w = resource_font(Font::big)->w * strlen(units_str[config_.unit]);
            const int16_t result_w = frame_w - (unit_w ? unit_w + 8 : 0);

            // Set up prompt text
            {
                constexpr int16_t h = 64;

                auto &wnd = ui.txt_prompt;
                wnd.set_enabled(false);
                wnd.set_font(Font::big);
                wnd.set_is_multiline(true);
                wnd.SetTextColor(COLOR_WHITE);
                wnd.SetAlignment(Align_t::RightBottom());
                wnd.SetText(prompt);
                wnd.SetRect(Rect16::fromLTWH(x, y, result_w, h));
                RegisterSubWin(wnd);

                y += h;
            }

            // Set up result text
            {
                auto &wnd = ui.txt_result;
                wnd.set_enabled(false);
                wnd.SetRect(Rect16::fromLTWH(x, y, result_w, result_h));
                RegisterSubWin(wnd);
            }

            // Set up unit text
            {
                auto &wnd = ui.txt_unit;
                wnd.set_enabled(false);
                wnd.set_font(Font::big);
                wnd.SetTextColor(COLOR_LIGHT_GRAY);
                wnd.SetAlignment(Align_t::LeftBottom());
                wnd.SetRect(Rect16::fromLTWH(x + frame_w - unit_w, y, unit_w, result_h - 6));
                wnd.SetText(string_view_utf8::MakeCPUFLASH(units_str[config_.unit]));
                RegisterSubWin(wnd);
            }

            y += result_h;

            // Set up limits text
            {
                constexpr auto h = 32;

                auto &wnd = ui.txt_limits;
                wnd.set_enabled(false);
                wnd.SetTextColor(COLOR_LIGHT_GRAY);
                wnd.SetAlignment(Align_t::RightTop());
                wnd.SetRect(Rect16::fromLTWH(x, y, result_w, h));

                StringBuilder b(limits_text_);
                b.append_float(config_.min_value, { .max_decimal_places = config_.max_decimal_places });
                b.append_string(" - ");
                b.append_float(config_.max_value, { .max_decimal_places = config_.max_decimal_places });
                wnd.SetText(string_view_utf8::MakeRAM(limits_text_.data()));

                RegisterSubWin(wnd);
            }
        }
    }

    is_special_value_ = (initial_value == config_.special_value);
    is_initial_value_ = true;
    result_accum_ = ResultAccumulator::from_float(initial_value, config);

    // Register cancel icon as the last thing to render it over the prompt
    RegisterSubWin(ui.btn_cancel);

    CaptureNormalWindow(*this);

    // We need to set focus to something, otherwise touch wouldn't work
    ui.btn_number[0].SetFocus();

    update();
}

void DialogNumericInput::screenEvent(window_t *sender, GUI_event_t event, void *const param) {
    switch (event) {

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        // Do not accept swipe gesture over the buttons, to prevent accidentaly going back
        if (get_child_by_touch_point(event_conversion_union { .pvoid = param }.point)) {
            return;
        }

        Sound_Play(eSOUND_TYPE::ButtonEcho);
        close(false);
        return;

    default:
        break;
    }

    IDialog::screenEvent(sender, event, param);
}

void DialogNumericInput::reset_special_values() {
    if (is_initial_value_ || is_special_value_) {
        is_initial_value_ = false;
        is_special_value_ = false;
        result_accum_ = {};
    }
}

void DialogNumericInput::update() {
    const auto accum_val = result_accum_.to_float();

    // Update result text
    {
        StringBuilder b(result_text_);
        result_accum_.to_string(b);

        auto &wnd = ui.txt_result;

        if (is_special_value_) {
            // Large font does not have string characters, so we cannot display special values with it
            wnd.set_font(Font::big);
            wnd.SetAlignment(Align_t::RightCenter());
            wnd.SetText(_(config_.special_value_str));
        } else {
            wnd.set_font(Font::large);
            wnd.SetAlignment(Align_t::RightBottom());
            wnd.SetText(string_view_utf8::MakeRAM(result_text_.data()));
        }

        // The text has still the same ref, we just changed the contents
        wnd.Invalidate();
    }

    // Check if we're within input bounds
    {
        const bool value_valid = ((accum_val >= config_.min_value) && (accum_val <= config_.max_value)) || is_special_value_;
        ui.txt_limits.SetTextColor(value_valid ? COLOR_GRAY : COLOR_ORANGE);
        ui.btn_ok.set_shadow(!value_valid);

        ui.txt_result.SetTextColor(is_initial_value_ ? COLOR_GRAY : COLOR_ORANGE);

        ui.txt_unit.set_visible(!is_special_value_);
    }

    // Update input buttons enable
    {
        const bool is_special_or_init_value = is_special_value_ || is_initial_value_;

        ui.btn_decimal.set_shadow((result_accum_.decimal_places.has_value() && !is_special_or_init_value) || !config_.max_decimal_places);

        // Disable backspace if we've cleared everything (accumulator is in its default vlaue)
        const bool is_default_value = (result_accum_ == ResultAccumulator {}) && (!config_.special_value.has_value() || is_special_value_);
        ui.btn_backspace.set_shadow(is_default_value);
        ui.btn_clear.set_shadow(is_default_value);

        // Allow toggling sign if we allow negative numbers in the input
        ui.btn_minus.set_shadow(config_.min_value >= 0);

        bool enable_numbers;
        if (is_special_or_init_value) {
            enable_numbers = true;

        } else if (result_accum_.decimal_places.has_value()) {
            enable_numbers = *result_accum_.decimal_places < config_.max_decimal_places;

        } else {
            const auto accum_digits = NumericInputConfig::num_digits(result_accum_.accum);
            const auto max_digits = config_.max_value_strlen({ .count_decimal_point = false, .count_minus_sign = false });
            enable_numbers = accum_digits < max_digits;
        }
        for (auto &btn : ui.btn_number) {
            btn.set_shadow(!enable_numbers);
        }
    }
}

void DialogNumericInput::close(bool success) {
    cancelled_ = !success;
    Screens::Access()->Close();
}

void DialogNumericInput::number_button_callback(window_t &button) {
    if (button.IsShadowed()) {
        return;
    }

    reset_special_values();

    const auto button_num = std::find_if(ui.btn_number.begin(), ui.btn_number.end(), [&](auto &a) { return &a == &button; }) - ui.btn_number.begin();

    // Accumulator accumulates both integral and decimal places.
    // If we're already at the decimal part (decimal_places is not std::nullopt), increase decimal places we're holding
    result_accum_.accum = result_accum_.accum * 10 + button_num;
    if (result_accum_.decimal_places) {
        (*result_accum_.decimal_places)++;
    }

    update();
}

void DialogNumericInput::minus_button_callback(window_t &button) {
    if (button.IsShadowed()) {
        return;
    }

    reset_special_values();
    result_accum_.negative ^= true; // xor 1 -> negation
    update();
}

void DialogNumericInput::decimal_button_callback(window_t &button) {
    if (button.IsShadowed()) {
        return;
    }

    reset_special_values();
    result_accum_.decimal_places = 0;
    update();
}

void DialogNumericInput::clear_button_callback(window_t &button) {
    if (button.IsShadowed()) {
        return;
    }

    result_accum_ = {};
    is_initial_value_ = false;
    is_special_value_ = config_.special_value.has_value();
    update();
}

void DialogNumericInput::backspace_button_callback(window_t &button) {
    if (button.IsShadowed()) {
        return;
    }

    // Update whatever we end up doing
    ScopeGuard _g = [this] {
        update();
    };
    is_initial_value_ = false;

    auto &a = result_accum_;

    // Take decimal digits
    if (a.decimal_places.has_value() && *a.decimal_places > 0) {
        (*a.decimal_places)--;
        a.accum /= 10;
        return;
    }

    // Take decimal point
    if (a.decimal_places.has_value()) {
        a.decimal_places = std::nullopt;
        return;
    }

    // Take standard digits
    if (a.accum > 0) {
        a.accum /= 10;
        return;
    }

    // Take minus sign
    if (a.negative) {
        a.negative = false;
        return;
    }

    // If we've reached here, we've cleared everything
    is_special_value_ = config_.special_value.has_value();
}

DialogNumericInput::ResultAccumulator DialogNumericInput::ResultAccumulator::from_float(float val, const NumericInputConfig &config) {
    ResultAccumulator r;

    if (val < 0) {
        r.negative = true;
        val = -val;
    }

    r.accum = floor(val);
    val -= r.accum;

    while (val != 0 && r.decimal_places.value_or(0) < config.max_decimal_places) {
        val *= 10;
        const auto flr = floor(val);
        r.accum = r.accum * 10 + flr;
        val -= flr;
        r.decimal_places = r.decimal_places.value_or(0) + 1;
    }

    return r;
}

float DialogNumericInput::ResultAccumulator::to_float() const {
    float result = static_cast<float>(accum);

    if (decimal_places.has_value()) {
        result /= static_cast<float>(pow(10.0f, *decimal_places));
    }

    if (negative) {
        result = -result;
    }

    return result;
}

void DialogNumericInput::ResultAccumulator::to_string(StringBuilder &b) const {
    if (negative) {
        b.append_char('-');

        if (accum == 0 && !decimal_places.has_value()) {
            return;
        }
    }

    auto exp = static_cast<int>(pow(10, decimal_places.value_or(0)));
    b.append_printf("%" PRIu32, accum / exp);

    if (decimal_places.has_value()) {
        b.append_char('.');

        auto r = accum % exp;
        while (exp > 1) {
            exp /= 10;
            b.append_char('0' + (r / exp));
            r %= exp;
        }
    }
}

window_aligned_t *DialogNumericInput::UI::button_window(ButtonType bt) {
    switch (bt) {

    case ButtonType::n0:
    case ButtonType::n1:
    case ButtonType::n2:
    case ButtonType::n3:
    case ButtonType::n4:
    case ButtonType::n5:
    case ButtonType::n6:
    case ButtonType::n7:
    case ButtonType::n8:
    case ButtonType::n9:
        return &btn_number[ftrstd::to_underlying(bt) - ftrstd::to_underlying(ButtonType::n0)];

    case ButtonType::decimal_point:
        return &btn_decimal;

    case ButtonType::minus:
        return &btn_minus;

    case ButtonType::clear:
        return &btn_clear;

    case ButtonType::backspace:
        return &btn_backspace;

    case ButtonType::ok:
        return &btn_ok;

    case ButtonType::none:
        return nullptr;
    }

    return nullptr;
}
