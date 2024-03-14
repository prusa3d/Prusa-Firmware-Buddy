/**
 * @file WindowMenuSwitch.hpp
 * @author Radek Vana
 * @brief WI_SWITCH == text version of WI_SPIN (non-numeric)
 * unlike WI_SPIN cannot be selected
 * todo try to inherit from WI_SPIN<const char**> lot of code could be reused
 * @date 2020-11-09
 */

#pragma once

#include "WindowMenuLabel.hpp"
#include "window_icon.hpp" //CalculateMinimalSize
#include <type_traits> //aligned_storage

/*****************************************************************************/
// IWiSwitch
class IWiSwitch : public AddSuper<WI_LABEL_t> {
public:
    static constexpr font_t *&BracketFont = GuiDefaults::FontMenuSpecial;
    static constexpr bool has_brackets = GuiDefaults::MenuSwitchHasBrackets;
    static constexpr padding_ui8_t Padding = GuiDefaults::MenuSwitchHasBrackets ? GuiDefaults::MenuPaddingSpecial : GuiDefaults::MenuPaddingItems;

    struct Items_t {
        enum class type_t : uint8_t {
            text,
            icon
        };
        union {
            string_view_utf8 *texts;
            const img::Resource **icon_resources;
        };
        uint16_t size;
        type_t type;

        // text ctor
        Items_t(string_view_utf8 array[], size_t SZ)
            : texts(array)
            , size(SZ)
            , type(type_t::text) {}

        // icon ctor
        Items_t(const img::Resource *array[], size_t SZ)
            : icon_resources(array)
            , size(SZ)
            , type(type_t::icon) {}
    };

protected:
    using TextMemSpace_t = std::aligned_storage<sizeof(string_view_utf8), alignof(string_view_utf8)>;
    using IconMemSpace_t = std::aligned_storage<sizeof(const img::Resource *), alignof(const img::Resource *)>;

    template <class T, class... E>
    Items_t FillArray(void *ArrayMem, E &&...e) {
        const size_t SZ = sizeof...(E);
        T *pArr = new (ArrayMem) T[SZ] { std::forward<E>(e)... };
        Items_t ret = Items_t(pArr, SZ);
        return ret;
    }

protected:
    size_t index;
    const Items_t items;

public:
    IWiSwitch(int32_t index, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, Items_t items_);

    void SetIndex(size_t idx);
    size_t GetIndex() const;

protected:
    static Rect16::Width_t calculateExtensionWidth(Items_t items, int32_t index);
    static Rect16::Width_t calculateExtensionWidth_text(Items_t items, int32_t index);
    static Rect16::Width_t calculateExtensionWidth_icon(Items_t items);

    void changeExtentionWidth();
    Rect16 getSwitchRect(Rect16 extension_rect) const;
    Rect16 getLeftBracketRect(Rect16 extension_rect) const;
    Rect16 getRightBracketRect(Rect16 extension_rect) const;

    virtual invalidate_t change(int dif) override;
    virtual void OnChange([[maybe_unused]] size_t old_index) {};
    virtual void click(IWindowMenu &window_menu) final;
    virtual void touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) final;
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;
    void printExtension_text(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const;
    void printExtension_icon(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const;
};

// I could use single template with type parameter and make aliases for WI_SWITCH_t and WI_ICON_SWITCH_t
// but I would have to rewrite all base ctor calls from ": WI_SWITCH_t(...)" to ": WI_SWITCH_t<N>(...)"
// I think it is caused by two phase lookup
template <size_t SZ>
class WI_SWITCH_t : public IWiSwitch {
    // properly aligned uninitialized storage for N T's
    typename TextMemSpace_t::type ArrayMemSpace[SZ];

public:
    template <class... E>
    WI_SWITCH_t(int32_t index, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&...e)
        : IWiSwitch(index, label, id_icon, enabled, hidden, FillArray<string_view_utf8>(&ArrayMemSpace, std::forward<E>(e)...)) {}
};

template <size_t SZ>
class WI_ICON_SWITCH_t : public IWiSwitch {
    // properly aligned uninitialized storage for N T's
    typename IconMemSpace_t::type ArrayMemSpace[SZ];

public:
    template <class... E>
    WI_ICON_SWITCH_t(int32_t index, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&...e)
        : IWiSwitch(index, label, id_icon, enabled, hidden, FillArray<const img::Resource *>(&ArrayMemSpace, std::forward<E>(e)...)) {}
};
