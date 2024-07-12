

#include "WindowItemFormatableSpin.hpp"

WI_LAMBDA_SPIN::WI_LAMBDA_SPIN(const string_view_utf8 &label, size_t item_count, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, size_t init_index, stdext::inplace_function<void(char *)> printAs)
    : WI_LAMBDA_LABEL_t(label, icon, enabled, hidden, printAs)
    , index(init_index)
    , item_count(item_count) {
    assert(item_count > init_index && "Cannot start with invalid value");

    UpdateText(); // Format text and get extension width
}

/**
 * @brief Update switch text and extension_width.
 */
void WI_LAMBDA_SPIN::UpdateText() {
    // Format switch text
    printAs(text);
    string_view_utf8 stringView = string_view_utf8::MakeRAM((uint8_t *)text);

    // Calculate extension width
    const size_t len = stringView.computeNumUtf8Chars();
    extension_width = width(GuiDefaults::FontMenuItems) * len + Padding.left + Padding.right + (GuiDefaults::MenuSwitchHasBrackets ? (width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right) * 2 : 0);
}

/**
 * @brief Get space just for the switch text.
 * @param extension_rect space for the entire switch text, including brackets
 * @return space for the switch text
 */
Rect16 WI_LAMBDA_SPIN::getSwitchRect(Rect16 extension_rect) const {
    if (!GuiDefaults::MenuSwitchHasBrackets) {
        return extension_rect;
    }

    extension_rect += Rect16::Left_t(width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    extension_rect -= Rect16::Width_t(width(BracketFont) * 2 + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

/**
 * @brief Get location of the left [ before switch text.
 * @param extension_rect space for the entire switch text, including brackets
 * @return space for the [
 */
Rect16 WI_LAMBDA_SPIN::getLeftBracketRect(Rect16 extension_rect) const {
    extension_rect = Rect16::Width_t(width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

/**
 * @brief Get location of the right ] after switch text.
 * @param extension_rect space for the entire switch text, including brackets
 * @return space for the ]
 */
Rect16 WI_LAMBDA_SPIN::getRightBracketRect(Rect16 extension_rect) const {
    extension_rect += Rect16::Left_t(extension_rect.Width() - (width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right));
    extension_rect = Rect16::Width_t(width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

/**
 * @brief Print switch text and brackets.
 */
void WI_LAMBDA_SPIN::printExtension(Rect16 extension_rect, Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {
    string_view_utf8 stringView = string_view_utf8::MakeRAM((uint8_t *)text);

    // Draw switch
    render_text_align(getSwitchRect(extension_rect), stringView, GuiDefaults::FontMenuItems, color_back,
        (IsFocused() && IsEnabled() && is_edited()) ? GuiDefaults::ColorSelected : color_text,
        padding_ui8(0, 4, 0, 0), Align_t::Center(), false);

    // Draw brackets
    if (GuiDefaults::MenuSwitchHasBrackets) {
        static constexpr uint8_t bf[] = "[";
        static constexpr uint8_t be[] = "]";
        render_text_align(getLeftBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(bf), BracketFont,
            color_back, (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, Align_t::Center(), false);

        render_text_align(getRightBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(be), BracketFont,
            color_back, (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, Align_t::Center(), false);
    }
}

/**
 * @brief Called when this item is clicked.
 * @param window_menu reference to menu where this item is shown
 */
void WI_LAMBDA_SPIN::click([[maybe_unused]] IWindowMenu &window_menu) {
    if (is_edited()) {
        OnClick(); // User overridable callback when item selection is confirmed
    }
    toggle_edit_mode();
}

/**
 * @brief Handle touch.
 * It behaves the same as click, but only when extension was clicked.
 * @param window_menu reference to menu where this item is shown
 * @param relative_touch_point where this item is touched
 */
void WI_LAMBDA_SPIN::touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    if (is_touch_in_extension_rect(window_menu, relative_touch_point)) {
        set_is_edited(true);
        SetIndex((index + 1) % item_count);
    }
}

void WI_LAMBDA_SPIN::SetIndex(size_t new_index) {
    if (new_index < item_count) {
        index = new_index;
        UpdateText(); // Update extension width
        InValidateLabel(); // Invalidate label to clear remaining longer text
        OnChange(); // User overridable callback when selection changes
        Invalidate();
    }
}

/**
 * @brief Selected value changed by dif ticks.
 * Called from parents.
 * @param dif change by this many ticks
 * @return yes if there is a change and the label is to be redrawn
 */
invalidate_t WI_LAMBDA_SPIN::change(int dif) {
    if (dif < 0) {
        if (index < static_cast<size_t>(-1 * dif)) {
            dif = -1 * index; // Prevent underflow
        }
    } else {
        if (index + dif >= item_count) {
            dif = item_count - 1 - index; // Prevent overflow
        }
    }

    if (dif == 0) {
        return invalidate_t::no; // No change
    }

    index += dif;
    UpdateText(); // Update extension width
    InValidateLabel(); // Invalidate label to clear remaining longer text
    OnChange(); // User overridable callback when selection changes

    return invalidate_t::yes;
}
