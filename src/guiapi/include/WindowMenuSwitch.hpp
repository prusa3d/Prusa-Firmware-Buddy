#pragma once

#include "i_window_menu_item.hpp"
#include "window_icon.hpp" //CalculateMinimalSize

#include <span>

// TODO: Create a utility subclass of MenuItemSelectMenu and move SZ > 3 use cases of this there
class MenuItemSwitch : public IWindowMenuItem {
public:
    static constexpr Font BracketFont = GuiDefaults::FontMenuSpecial;
    static constexpr bool has_brackets = GuiDefaults::MenuSwitchHasBrackets;
    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSwitchHasBrackets ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPaddingItems;

public:
    MenuItemSwitch(const string_view_utf8 &label, const std::span<const char *const> &items, size_t initial_index = 0);

    inline void set_translate_items(bool set) {
        translate_items_ = set;
    }

    void set_index(size_t set);

    inline size_t item_count() const {
        return items_.size();
    }

    string_view_utf8 current_item_text() const;

    inline size_t GetIndex() const {
        return index;
    }

    /// DEPRECATED
    inline void SetIndex(size_t set) {
        set_index(set);
    }

protected:
    Rect16 getSwitchRect(Rect16 extension_rect) const;
    Rect16 getLeftBracketRect(Rect16 extension_rect) const;
    Rect16 getRightBracketRect(Rect16 extension_rect) const;

    Rect16::Width_t calculateExtensionWidth() const;
    void changeExtentionWidth();

    virtual invalidate_t change(int dif) override;
    virtual void OnChange([[maybe_unused]] size_t old_index) {};
    virtual void click(IWindowMenu &window_menu) final;
    virtual void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

protected:
    size_t index = 0;

private:
    std::span<const char *const> items_;
    bool translate_items_ = true;
};
