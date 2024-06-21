#pragma once

#include <window_icon.hpp>
#include <window_text.hpp>
#include <IDialog.hpp>
#include <numeric_input_config.hpp>
#include <str_utils.hpp>
#include <option/has_touch.h>

#if HAS_TOUCH()
    #include <touchscreen/touchscreen.hpp>
#endif

class DialogNumericInput : public IDialog {

public:
    using Config = NumericInputConfig;
    using Result = std::optional<double>;

    static constexpr int matrix_cols = 3;
    static constexpr int matrix_rows = 4;
    static constexpr int matrix_count = matrix_cols * matrix_rows;

    enum class ButtonType {
        none,
        minus,
        decimal_point,
        clear,
        backspace,
        ok,
        _special_cnt,

        n0 = _special_cnt,
        n1,
        n2,
        n3,
        n4,
        n5,
        n6,
        n7,
        n8,
        n9,
    };

public:
    /// Executes the numeric input dialog.
    /// \returns the entered number or \p nullopt if the dialog was cancelled
    static Result exec(const string_view_utf8 &prompt, double initial_value, const Config &config);

    inline Result result() const;

protected:
    DialogNumericInput(const string_view_utf8 &prompt, double initial_value, const Config &config);

protected:
    void screenEvent(window_t *sender, GUI_event_t event, void *const param) override;

private:
    void reset_special_values();
    void update();

    void close(bool success);

private:
    void number_button_callback(window_t &button);
    void minus_button_callback(window_t &button);
    void decimal_button_callback(window_t &button);
    void clear_button_callback(window_t &button);
    void backspace_button_callback(window_t &button);

private:
    const string_view_utf8 prompt_;
    const Config &config_;

    struct ResultAccumulator {

    public:
        /// Number accumulator, including the decimal places
        uint32_t accum = 0;

        /// Number of decimal places the accumulator holds.
        std::optional<uint8_t> decimal_places;

        bool negative = false;

    public:
        static ResultAccumulator from_float(float val, const NumericInputConfig &config);
        float to_float() const;

        void to_string(StringBuilder &b) const;

    public:
        bool operator==(const ResultAccumulator &) const = default;
    };
    ResultAccumulator result_accum_;
    std::array<char, 10> result_text_;
    std::array<char, 16> limits_text_;

    bool cancelled_ : 1 = false;

    bool is_special_value_ : 1 = false;
    bool is_initial_value_ : 1 = true;

#if HAS_TOUCH()
    /// Don't be as strict for detecting touch clicks on this keyboard
    Touchscreen_Base::LenientClickGuard lenient_click_guard_;
#endif

private:
    struct UI {

    public:
        window_aligned_t *button_window(ButtonType bt);

    public:
        std::array<window_text_button_t, 10> btn_number;
        window_text_button_t btn_decimal;
        window_icon_button_t btn_minus;
        window_icon_button_t btn_clear;
        window_icon_button_t btn_backspace;
        window_icon_button_t btn_ok;
        window_icon_button_t btn_cancel;

        window_text_t txt_prompt;
        window_text_t txt_result;
        window_text_t txt_unit;
        window_text_t txt_limits;
    };
    UI ui;
};
