#pragma once

#include <array>
#include <window_text.hpp>
#include <window_icon.hpp>
#include <IDialog.hpp>
#include <option/has_touch.h>

#if HAS_TOUCH()
    #include <touchscreen/touchscreen.hpp>
#endif

#include "dialog_text_input_layout.hpp"

namespace dialog_text_input {

struct ButtonsLayout;

/// Dialog with a keyboard that allows inputting text
class DialogTextInput final : public IDialog {

public:
    /**
     * Executes the text input dialog on the provided text buffer.
     * \param buffer is taken for initial value and for storing the result.
     * \returns true if user pressed ok, false if cancelled.
     *
     * The buffer can get changed even if the user pressed cancel!
     */
    static bool exec(const string_view_utf8 &prompt, std::span<char> buffer);

protected:
    DialogTextInput(const string_view_utf8 &prompt, std::span<char> buffer);

protected:
    void screenEvent(window_t *sender, GUI_event_t event, void *const param) final;

private:
    void setup_ui();

    /// Updates what buttons are enabled and disabled
    void update_ui_enabled();

    void set_keyboard_layout(const ButtonsLayout &layout);

    void button_callback(window_t &button);

    /// Finishes edit char and appends it to the buffer
    void flush_edit_char();

    void update_result();

private:
    std::span<char> buffer_;

    // edited char + terminating \0
    std::array<char, 2> edit_char_buffer_ = { 0, 0 };

    const ButtonsLayout *current_layout_ = nullptr;

    /// Time of last edit of the char buffer (for char timeout purposes)
    uint32_t last_char_edit_ms_ = 0;

    bool cancelled_ = false;

#if HAS_TOUCH()
    /// Don't be as strict for detecting touch clicks on this keyboard
    Touchscreen_Base::LenientClickGuard lenient_click_guard_;
#endif

private:
    struct UI {

    public:
        window_text_t txt_prompt;

        /// Label displaying the currently edited text
        window_text_t txt_result;

        /// Label for currently edited char
        window_text_t txt_edit_char;

        WindowButton btn_matrix[button_count];

    } ui;
};
} // namespace dialog_text_input

using DialogTextInput = dialog_text_input::DialogTextInput;
