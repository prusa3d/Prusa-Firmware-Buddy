
#pragma once
#include "WindowItemFormatableLabel.hpp"
#include "guitypes.hpp"
#include <inplace_function.hpp>

/**
 * @brief Spin switch with texts printed with lambda.
 */
class WI_LAMBDA_SPIN : public WI_LAMBDA_LABEL_t {
    static constexpr Font BracketFont = GuiDefaults::FontMenuSpecial;
    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSwitchHasBrackets ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPaddingItems;

    size_t index; ///< Currently selected index
    std::array<char, GuiDefaults::infoDefaultLen> text; ///< Buffer for switch text

public:
    /**
     * @brief Construct a spinner with different texts to choose from.
     * @param item_count number of valid indexes, valid are from 0 to index_n - 1
     */
    WI_LAMBDA_SPIN(const string_view_utf8 &label, size_t item_count, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, size_t init_index, const PrintFunction &printAs);

    /**
     * @brief Get currently selected index.
     * This is to be used in child in printAs() to print a correct label.
     */
    size_t GetIndex() const { return index; };

    /**
     * @brief Set currently selected index.
     */
    void SetIndex(size_t new_index);

protected:
    /**
     * @brief Update switch text and extension_width.
     */
    void UpdateText();

    /**
     * @brief Get space just for the switch text.
     * @param extension_rect space for the entire switch text, including brackets
     * @return space for the switch text
     */
    Rect16 getSwitchRect(Rect16 extension_rect) const;

    /**
     * @brief Get location of the left [ before switch text.
     * @param extension_rect space for the entire switch text, including brackets
     * @return space for the [
     */
    Rect16 getLeftBracketRect(Rect16 extension_rect) const;

    /**
     * @brief Get location of the right ] after switch text.
     * @param extension_rect space for the entire switch text, including brackets
     * @return space for the ]
     */
    Rect16 getRightBracketRect(Rect16 extension_rect) const;

    /**
     * @brief Print switch text and brackets.
     */
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const override;

    /**
     * @brief Called when this item is clicked.
     * @param window_menu reference to menu where this item is shown
     */
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) final;

    /**
     * @brief Selected value changed by dif ticks.
     * Called from parents.
     * @param dif change by this many ticks
     * @return yes if there is a change and the label is to be redrawn
     */
    virtual invalidate_t change(int dif) override;

    /**
     * @brief Called when the spin value is confirmed by a second click.
     * To be overriden in children.
     */
    virtual void OnClick() {}

    /**
     * @brief Called when the spin value is changed.
     * To be overriden in children.
     */
    virtual void OnChange() {}

protected:
    size_t item_count; ///< Limit for the spinner switch index.
};
