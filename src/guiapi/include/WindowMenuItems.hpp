#pragma once

#include "IWindowMenuItem.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "super.hpp"
#include "i18n.h"

#define UNIT_RECT_CHAR_WIDTH   4
#define UNIT_HALFSPACE_PADDING 6
#define SWITCH_ICON_RECT_WIDTH 40

//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no);
    virtual bool Change(int dif) override;
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const override;
    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override;
};

//IWiSpin
class IWiSpin : public AddSuper<IWindowMenuItem> {
protected:
    enum { WIO_MIN = 0,
        WIO_MAX = 1,
        WIO_STEP = 2 };
    static std::array<char, 10> temp_buff; //temporary buffer to print value for text measurements
    virtual void click(IWindowMenu &window_menu) final;
    // Old GUI implementation (is used in derived classes in printItem instead of derived func, when we want old GUI)
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual char *sn_prt() const = 0;

public:
    IWiSpin(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : AddSuper<IWindowMenuItem>(label, id_icon, enabled, hidden) {}
    virtual void OnClick() {}
    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override;
};

//WI_SPIN
template <class T>
class WI_SPIN_t : public AddSuper<IWiSpin> {

public: //todo private
    T value;
    const T *range;
    const char *prt_format;
    const char *units;

protected:
    virtual char *sn_prt() const override;
    /* getMenuRects (not derived because of New/Old GUI and 3 rect array return (not 2))
    *   Returns rect array according to this format: [label], [value], [units]
    */
    std::array<Rect16, 3> getSpinMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
        Rect16 base_rect = getRollingRect(window_menu, rect);
        Rect16 unit_rect = { int16_t(base_rect.Left() + base_rect.Width() - UNIT_RECT_CHAR_WIDTH * window_menu.secondary_font->w), base_rect.Top(), uint16_t(UNIT_RECT_CHAR_WIDTH * window_menu.secondary_font->w), base_rect.Height() };
        base_rect -= unit_rect.Width();
        char *buff = sn_prt();
        Rect16 spin_rect = getCustomRect(window_menu, base_rect, strlen(buff) * window_menu.secondary_font->w);
        base_rect -= spin_rect.Width();

        return std::array<Rect16, 3> { base_rect, spin_rect, unit_rect };
    }
    /* derived printItem()
    * Adition of new implementation:
    *   values are offset from right (4 chars) and they are in secondary_font (smaller)
    *   optional value units are added next to values (gray color)
    */
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override {
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        std::array<Rect16, 2> rects = IWiSpin::getMenuRects(window_menu, rect); // MINI GUI implementation
#else                                                                           // PRINTER TYPE != PRUSA_PRINTER_MINI
        std::array<Rect16, 3> rects = getSpinMenuRects(window_menu, rect);
#endif                                                                          // PRINTER TYPE == PRUSA_PRINTER_MINI
        //draw label
        printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
        //draw spin
        // this MakeRAM is safe - temp_buff is allocated for the whole life of IWiSpin
        render_text_align(rects[1], string_view_utf8::MakeRAM((const uint8_t *)temp_buff.data()),
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
            window_menu.font,
            color_back, IsSelected() ? COLOR_ORANGE : color_text,
            window_menu.padding,
#else  // PRINTER TYPE != PRUSA_PRINTER_MINI
            window_menu.secondary_font,
            color_back, IsSelected() ? COLOR_ORANGE : color_text,
            padding_ui8(0, 6, 0, 0),
#endif // PRINTER TYPE == PRUSA_PRINTER_MINI
            window_menu.GetAlignment());

#if (PRINTER_TYPE != PRINTER_PRUSA_MINI)
        uint8_t unit_padding_left = units[0] == '\177' ? 0 : UNIT_HALFSPACE_PADDING; // °C are not separated with halfspace
        render_text_align(rects[2], string_view_utf8::MakeRAM((const uint8_t *)units), window_menu.secondary_font, color_back,
            COLOR_SILVER, padding_ui8(unit_padding_left, 6, 0, 0), window_menu.GetAlignment());
#endif // (PRINTER_TYPE != PRINTER_PRUSA_MINI
    }

public:
    WI_SPIN_t(T value, const char *units, const T *range, const char *prt_format, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual bool Change(int dif) override;
    void ClrVal() { value = static_cast<T>(0); }

    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override {
        reInitRoll(window_menu, getSpinMenuRects(window_menu, rect)[0]);
    }
};

//WI_SPIN_I08_t
//defines print format for int8_t version of WI_SPIN_t
class WI_SPIN_I08_t : public WI_SPIN_t<int8_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I08_t(int8_t value, const char *units, const int8_t *range, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WI_SPIN_t<int8_t>(value, units, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_I16_t
//defines print format for int16_t version of WI_SPIN_t
class WI_SPIN_I16_t : public WI_SPIN_t<int16_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I16_t(int16_t value, const char *units, const int16_t *range, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WI_SPIN_t<int16_t>(value, units, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_I32_t
//defines print format for int32_t version of WI_SPIN_t
class WI_SPIN_I32_t : public WI_SPIN_t<int32_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I32_t(int32_t value, const char *units, const int32_t *range, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WI_SPIN_t<int32_t>(value, units, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U08_t
//defines print format for uint8_t version of WI_SPIN_t
class WI_SPIN_U08_t : public WI_SPIN_t<uint8_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U08_t(uint8_t value, const char *units, const uint8_t *range, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WI_SPIN_t<uint8_t>(value, units, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U16_t
//defines print format for uint16_t version of WI_SPIN_t
class WI_SPIN_U16_t : public WI_SPIN_t<uint16_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U16_t(uint16_t value, const char *units, const uint16_t *range, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WI_SPIN_t<uint16_t>(value, units, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U32_t
//defines print format for uint32_t version of WI_SPIN_t
class WI_SPIN_U32_t : public WI_SPIN_t<uint32_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U32_t(uint32_t value, const char *units, const uint32_t *range, string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WI_SPIN_t<uint32_t>(value, units, range, prt_format, label, id_icon, enabled, hidden) {}
};

//todo inherit from WI_SPIN_t<const char**>
class IWiSwitch : public AddSuper<IWindowMenuItem> {
public:
    size_t index; //todo private
    IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden);
    virtual void ClrIndex() { index = 0; }
    virtual size_t size() = 0;
    virtual bool Change(int dif) override;
    virtual bool SetIndex(size_t idx);

protected:
    virtual void OnChange(size_t old_index) = 0;
    virtual void click(IWindowMenu &window_menu) final;
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual const char *get_item() const = 0;
};

/*****************************************************************************/
//WI_SWITCH == text version of WI_SPIN (non-numeric)
//unlike WI_SPIN cannot be selected
//todo try to inherit from WI_SPIN<const char**> lot of code could be reused
template <size_t SZ>
class WI_SWITCH_t : public AddSuper<IWiSwitch> {
public: //todo private
    const std::array<const char *, SZ> items;
    virtual size_t size() override { return items.size(); }

public:
    //cannot create const std::array<const char *, SZ> with std::initializer_list<const char*>
    //template<class ...E> and {{std::forward<E>(e)...}} is workaround
    template <class... E>
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden)
        , items { { std::forward<E>(e)... } } {}

    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override {
        reInitRoll(window_menu, getSwitchMenuRects(window_menu, rect)[0]);
    }

protected:
    virtual const char *get_item() const override { return items[index]; }

    /**
    * Returns Menu item rect devided into 4 rects according to this format: [label], ['['], [switch], [']']
    **/
    std::array<Rect16, 4> getSwitchMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
        Rect16 base_rect = getRollingRect(window_menu, rect);
        Rect16 switch_rect, bracket_start, bracket_end;

        bracket_end = { int16_t(base_rect.Left() + base_rect.Width() - window_menu.secondary_font->w), base_rect.Top(), uint16_t(window_menu.secondary_font->w), base_rect.Height() };
        base_rect -= bracket_end.Width();

        string_view_utf8 localizedItem = _(get_item());
        switch_rect = getCustomRect(window_menu, base_rect, localizedItem.computeNumUtf8CharsAndRewind() * window_menu.secondary_font->w);
        base_rect -= switch_rect.Width();

        bracket_start = { int16_t(base_rect.Left() + base_rect.Width() - window_menu.secondary_font->w), base_rect.Top(), uint16_t(window_menu.secondary_font->w), base_rect.Height() };
        base_rect -= bracket_start.Width();

        return std::array<Rect16, 4> { base_rect, bracket_start, switch_rect, bracket_end };
    }

    /* derived printItem()
    *  Aditions to the new implementation:
    *   every switch value is scoped in gray brackets []
    *   Secondary_font (smaller) is used on all items on right
    */
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override {
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        std::array<Rect16, 2> rects = IWiSwitch::getMenuRects(window_menu, rect);
#else  // PRINTER_TYPE != PRINTER_PRUSA_MINI
        std::array<Rect16, 4> rects = getSwitchMenuRects(window_menu, rect);
#endif // PRINTER_TYPE == PRINTER_PRUSA_MINI

        //draw label
        printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        render_text_align(rects[1], _(get_item()), window_menu.font,
            color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.GetAlignment());
#else  // PRINTER_TYPE != PRINTER_PRUSA_MINI
        render_text_align(rects[1], _("["), window_menu.secondary_font,
            color_back, COLOR_SILVER, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());
        //draw switch
        render_text_align(rects[2], _(get_item()), window_menu.secondary_font,
            color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());
        //draw bracket end  TODO: Change font
        render_text_align(rects[3], _("]"), window_menu.secondary_font,
            color_back, COLOR_SILVER, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());
#endif // PRINTER_TYPE == PRINTER_PRUSA_MINI
    }
};

//most common version of WI_SWITCH with on/off options
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = N_("Off");
    constexpr static const char *str_On = N_("On");

public:
    WI_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, str_Off, str_On) {}
};

template <size_t SZ>
class WI_ICON_SWITCH_t : public AddSuper<IWiSwitch> {
public: //todo private
    const std::array<uint16_t, SZ> items;
    virtual size_t size() override { return items.size(); }

public:
    template <class... E>
    WI_ICON_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden)
        , items { { std::forward<E>(e)... } } {}

    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) override {
        reInitRoll(window_menu, getMenuRects(window_menu, rect)[0]);
    }

protected:
    virtual const char *get_item() const override { return 0; } // TODO: useless here (but we need virtual get_item in iWiSwitch)
    // Returns selected icon id
    const uint16_t get_icon() const { return items[index]; }
    // Returns array of rects in this format: [label], [icon]
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override {
        Rect16 base_rect = getRollingRect(window_menu, rect);
        Rect16 icon_rect = { 0, 0, 0, 0 };
        if (get_icon()) { // WI_ICON_SWITCH
            icon_rect = { int16_t(base_rect.Left() + base_rect.Width() - SWITCH_ICON_RECT_WIDTH), base_rect.Top(), SWITCH_ICON_RECT_WIDTH, base_rect.Height() };
        }
        base_rect -= icon_rect.Width();
        return std::array<Rect16, 2> { base_rect, icon_rect };
    }

    /*
    * derived printItem()
    *   prints label and selected icon
    */
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override {
        std::array<Rect16, 2> rects = getMenuRects(window_menu, rect);
        //draw label
        printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
        //draw on/off icon
        render_icon_align(rects[1], get_icon(), window_menu.color_back, RENDER_FLG(ALIGN_CENTER, swap));
    }
};

class WI_ICON_SWITCH_OFF_ON_t : public WI_ICON_SWITCH_t<2> {
    constexpr static const uint16_t iid_off = IDR_PNG_switch_off_36px;
    constexpr static const uint16_t iid_on = IDR_PNG_switch_on_36px;

public:
    WI_ICON_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_ICON_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, iid_off, iid_on) {}
};

//currently broken todo FIXME
//WI_SELECT == switch with no label
//but can be selected like WI_SPIN
class WI_SELECT_t : public IWindowMenuItem {
public: //todo private
    uint32_t index;
    const char **strings;

protected:
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual bool Change(int dif) override;
};

/*****************************************************************************/
//template definitions
//WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T value, const char *spin_units, const T *range, const char *prt_format, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : AddSuper<IWiSpin>(label, id_icon, enabled, hidden)
    , value(value)
    , range(range)
    , prt_format(prt_format)
    , units(spin_units) {}

template <class T>
bool WI_SPIN_t<T>::Change(int dif) {
    T old = value;
    value += (T)dif * range[WIO_STEP];
    value = dif >= 0 ? std::max(value, old) : std::min(value, old); //check overflow/underflow
    value = std::min(value, range[WIO_MAX]);
    value = std::max(value, range[WIO_MIN]);
    return old != value;
}

template <class T>
char *WI_SPIN_t<T>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), prt_format, value);
    return temp_buff.data();
}

template <>
inline char *WI_SPIN_t<float>::sn_prt() const {
    snprintf(temp_buff.data(), temp_buff.size(), prt_format, static_cast<double>(value));
    return temp_buff.data();
}

/*****************************************************************************/
//template definitions

/*****************************************************************************/
//advanced types
class MI_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Return");

public:
    MI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};

class MI_TEST_DISABLED_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = "Disabled RETURN button";

public:
    MI_TEST_DISABLED_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};
