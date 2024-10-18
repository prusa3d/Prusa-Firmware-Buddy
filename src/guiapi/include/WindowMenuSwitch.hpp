/**
 * @file WindowMenuSwitch.hpp
 * @author Radek Vana
 * @brief WI_SWITCH == text version of WI_SPIN (non-numeric)
 * unlike WI_SPIN cannot be selected
 * todo try to inherit from WI_SPIN<const char**> lot of code could be reused
 * @date 2020-11-09
 */

#pragma once

#include "i_window_menu_item.hpp"
#include "window_icon.hpp" //CalculateMinimalSize

#include <span>

/*****************************************************************************/
// IWiSwitch
class IWiSwitch : public IWindowMenuItem {
public:
    static constexpr Font BracketFont = GuiDefaults::FontMenuSpecial;
    static constexpr bool has_brackets = GuiDefaults::MenuSwitchHasBrackets;
    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSwitchHasBrackets ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPaddingItems;

protected:
    size_t index = 0;

public:
    // !!! Call changeExtentionWidth() after the items are initialized in the child
    IWiSwitch(const string_view_utf8 &label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);

    void SetIndex(size_t idx);

    inline size_t GetIndex() const {
        return index;
    }

    virtual size_t item_count() const = 0;
    virtual string_view_utf8 current_item_text() const = 0;

protected:
    Rect16::Width_t calculateExtensionWidth() const;
    void changeExtentionWidth();

    Rect16 getSwitchRect(Rect16 extension_rect) const;
    Rect16 getLeftBracketRect(Rect16 extension_rect) const;
    Rect16 getRightBracketRect(Rect16 extension_rect) const;

    virtual invalidate_t change(int dif) override;
    virtual void OnChange([[maybe_unused]] size_t old_index) {};
    virtual void click(IWindowMenu &window_menu) final;
    virtual void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;
};

/// IWiSwitch implementation with fixed number of fixed items, stored in a buffer
/// TODO: Most of the usage can be shifted to a nicer WiEnumSwitch
template <size_t SZ>
class WI_SWITCH_t : public IWiSwitch {

public:
    template <class... E>
    WI_SWITCH_t(size_t index, const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&...e)
        : IWiSwitch(label, id_icon, enabled, hidden)
        , items_ { std::forward<E>(e)... } //
    {
        SetIndex(index);

        // Items are initialized now, update extension width
        changeExtentionWidth();
    }

    inline size_t item_count() const final {
        return SZ;
    }
    inline string_view_utf8 current_item_text() const final {
        return items_[index];
    }

private:
    std::array<string_view_utf8, SZ> items_;
};

class MenuItemSwitch : public IWiSwitch {

public:
    MenuItemSwitch(const string_view_utf8 &label, const std::span<const char *const> &items, size_t current_index = 0)
        : IWiSwitch(label)
        , items_(items) //
    {
        SetIndex(current_index);

        // Items are initialized now, update extension width
        changeExtentionWidth();
    }

    inline void set_translate_items(bool set) {
        translate_items_ = set;
    }

    inline size_t item_count() const final {
        return items_.size();
    }
    inline string_view_utf8 current_item_text() const final {
        const char *str = items_[index];
        return translate_items_ ? _(str) : string_view_utf8::MakeRAM(str);
    }

private:
    std::span<const char *const> items_;
    bool translate_items_ = true;
};
