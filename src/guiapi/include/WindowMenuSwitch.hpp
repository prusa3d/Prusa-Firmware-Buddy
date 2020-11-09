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

    struct Items_t {
        enum class type_t : uint8_t {
            text,
            picture
        };
        union {
            string_view_utf8 *texts;
            uint16_t *picture_resources;
        };
        uint16_t size;
        type_t type;

        template <size_t SZ>
        Items_t(std::array<string_view_utf8, SZ> &array)
            : texts(array.data())
            , size(SZ)
            , type(type_t::text) {}
    };

protected:
    template <size_t SZ>
    using ArrayMemSpace_t = std::aligned_storage<sizeof(std::array<string_view_utf8, SZ>), 1>;

    template <size_t SZ>
    using Array_t = std::array<string_view_utf8, SZ>;

    template <size_t SZ, class... E>
    Items_t FillArray(ArrayMemSpace_t<SZ> *ArrayMem, E &&... e) {
        Array_t<SZ> *pArr = new (ArrayMem) Array_t<SZ>({ std::forward<E>(e)... });
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

    Rect16 getSwitchRect(Rect16 extension_rect) const;
    Rect16 getLeftBracketRect(Rect16 extension_rect) const;
    Rect16 getRightBracketRect(Rect16 extension_rect) const;

    virtual void OnChange(size_t old_index) = 0;
    virtual void click(IWindowMenu &window_menu) final;
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const override;
};

/*****************************************************************************/
//WI_SWITCH == text version of WI_SPIN (non-numeric)
//unlike WI_SPIN cannot be selected
//todo try to inherit from WI_SPIN<const char**> lot of code could be reused

//using WI_SWITCH_t = IWiSwitch;
template <size_t SZ>
class WI_SWITCH_t : public IWiSwitch {
    ArrayMemSpace_t<SZ> ArrayMemSpace;

public:
    //cannot create const std::array<const char *, SZ> with std::initializer_list<const char*>
    //template<class ...E> and {{std::forward<E>(e)...}} is workaround
    template <class... E>
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : IWiSwitch(index, label, id_icon, enabled, hidden, FillArray<SZ>(&ArrayMemSpace, std::forward<E>(e)...)) {}
};

#if 0
template <size_t SZ>
class WI_SWITCH_t : public AddSuper<IWiSwitch> {
public: //todo private
    //using Array = const std::array<string_view_utf8, SZ>;//do not erase this commented code, solution storing string_view_utf8 instead const char*
    using Array = const std::array<const char *, SZ>;
    const Array array;

    //cannot create const std::array<const char *, SZ> with std::initializer_list<const char*>
    //template<class ...E> and {{std::forward<E>(e)...}} is workaround
    template <class... E>
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden, calculateExtensionWidth(Array({ std::forward<E>(e)... })))
        , array { { std::forward<E>(e)... } } {}

    //do not erase this commented code, solution storing string_view_utf8 instead const char*
    /*
    static size_t MaxNumberOfCharacters(const Array>& items) {
        size_t max_len = 0;
        for (size_t i = 0; i < SZ; ++i) {
            size_t len = array[i].computeNumUtf8CharsAndRewind();
            if (len > max_len) max_len = len;
        }
        return IWiSwitch::calculateExtensionWidth(max_len);
    }

    //ctor must receive translated texts
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden)
        , array { { std::forward<E>(e)... } } {}
    */
protected:
    static size_t calculateExtensionWidth(const Array &array) {
        return IWiSwitch::calculateExtensionWidth(Items_t ({array.data(),array.size()}));
    }

    virtual Items_t get_items() const override {
        const Items_t ret = { items.data(), items.size() };
        return ret;
    }
};

#endif
/*
template <size_t SZ>
class WI_ICON_SWITCH_t : public AddSuper<IWiSwitch> {
protected:
    using Array = const std::array<uint16_t, SZ>;
    const Array items;

    static size_t calculateExtensionWidth(const Array &items) {
        size_t max_width = 0;
        for (size_t i = 0; i < SZ; ++i) {
            size_t width = window_icon_t::CalculateMinimalSize(items[i]).w;
            if (width > max_width)
                max_width = width;
        }
        return max_width;
    }

public:
    template <class... E>
    WI_ICON_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden, calculateExtensionWidth(Array({ std::forward<E>(e)... })))
        , items { { std::forward<E>(e)... } } {}

protected:
    virtual Items_t get_items() const override {
        const Items_t ret = { nullptr, items.size() };
        return ret;
    }
    // Returns selected icon id
    const uint16_t get_icon() const { return items[index]; }

    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const override {
        //draw on/off icon
        render_icon_align(extension_rect, get_icon(), GuiDefaults::MenuColorBack, RENDER_FLG(ALIGN_CENTER, swap));
    }
};
*/
