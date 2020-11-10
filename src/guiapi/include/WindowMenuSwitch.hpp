/**
 * @file WindowMenuSwitch.hpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-09
 *
 * @copyright Copyright (c) 2020
 *
 */

#pragma once

#include "WindowMenuLabel.hpp"
#include "window_icon.hpp" //CalculateMinimalSize
#include <type_traits>     //aligned_storage

/*****************************************************************************/
//IWiSwitch
class IWiSwitch : public AddSuper<WI_LABEL_t> {
public:
    static constexpr font_t *&BracketFont = GuiDefaults::FontMenuSpecial;
    static constexpr bool has_brackets = GuiDefaults::MenuSwitchHasBrackets;
    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSwitchHasBrackets ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPadding;

    struct Items_t {
        enum class type_t : uint8_t {
            text,
            icon
        };
        union {
            string_view_utf8 *texts;
            uint16_t *icon_resources;
        };
        uint16_t size;
        type_t type;

        //text ctor
        template <size_t SZ>
        Items_t(std::array<string_view_utf8, SZ> &array)
            : texts(array.data())
            , size(SZ)
            , type(type_t::text) {}

        //icon ctor
        template <size_t SZ>
        Items_t(std::array<uint16_t, SZ> &array)
            : icon_resources(array.data())
            , size(SZ)
            , type(type_t::icon) {}
    };

protected:
    //text template definitions
    template <size_t SZ>
    using TextArrayMemSpace_t = std::aligned_storage<sizeof(std::array<string_view_utf8, SZ>), 1>;

    template <size_t SZ>
    using TextArray_t = std::array<string_view_utf8, SZ>;

    template <size_t SZ, class... E>
    Items_t FillTextArray(TextArrayMemSpace_t<SZ> *ArrayMem, E &&... e) {
        TextArray_t<SZ> *pArr = new (ArrayMem) TextArray_t<SZ>({ std::forward<E>(e)... });
        Items_t ret = Items_t(*pArr);
        return ret;
    }

    //icon template definitions
    template <size_t SZ>
    using IconArrayMemSpace_t = std::aligned_storage<sizeof(std::array<uint16_t, SZ>), 1>;

    template <size_t SZ>
    using IconArray_t = std::array<uint16_t, SZ>;

    template <size_t SZ, class... E>
    Items_t FillIconArray(IconArrayMemSpace_t<SZ> *ArrayMem, E &&... e) {
        IconArray_t<SZ> *pArr = new (ArrayMem) IconArray_t<SZ>({ std::forward<E>(e)... });
        Items_t ret = Items_t(*pArr);
        return ret;
    }

protected:
    size_t index;
    const Items_t items;

public:
    IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, Items_t items_);

    virtual invalidate_t Change(int dif) override;
    bool SetIndex(size_t idx);

protected:
    static Rect16::Width_t calculateExtensionWidth(Items_t items);
    static Rect16::Width_t calculateExtensionWidth_text(Items_t items);
    static Rect16::Width_t calculateExtensionWidth_icon(Items_t items);

    Rect16 getSwitchRect(Rect16 extension_rect) const;
    Rect16 getLeftBracketRect(Rect16 extension_rect) const;
    Rect16 getRightBracketRect(Rect16 extension_rect) const;

    virtual void OnChange(size_t old_index) = 0;
    virtual void click(IWindowMenu &window_menu) final;
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const override;
    void printExtension_text(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const;
    void printExtension_icon(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const;
};

/*****************************************************************************/
//WI_SWITCH == text version of WI_SPIN (non-numeric)
//unlike WI_SPIN cannot be selected
//todo try to inherit from WI_SPIN<const char**> lot of code could be reused

//using WI_SWITCH_t = IWiSwitch;
template <size_t SZ>
class WI_SWITCH_t : public IWiSwitch {
    TextArrayMemSpace_t<SZ> ArrayMemSpace;

public:
    template <class... E>
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : IWiSwitch(index, label, id_icon, enabled, hidden, FillTextArray<SZ>(&ArrayMemSpace, std::forward<E>(e)...)) {}
};

template <size_t SZ>
class WI_ICON_SWITCH_t : public IWiSwitch {
    IconArrayMemSpace_t<SZ> ArrayMemSpace;

public:
    template <class... E>
    WI_ICON_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : IWiSwitch(index, label, id_icon, enabled, hidden, FillIconArray<SZ>(&ArrayMemSpace, std::forward<E>(e)...)) {}
};
