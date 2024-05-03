#include "dialog_text_input.hpp"

bool DialogTextInput::exec(string_view_utf8, std::span<char>) {
    // TODO: Add text keyboard for Mini.
    // The main problem being that it doesn't fit in the FLASH :/
    return false;
}
