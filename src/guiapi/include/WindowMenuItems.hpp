#pragma once

#include "IWindowMenuItem.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "super.hpp"
#include "i18n.h"

//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif) override;
};

//IWiSpin
class IWiSpin : public AddSuper<IWindowMenuItem> {
protected:
    enum { WIO_MIN = 0,
        WIO_MAX = 1,
        WIO_STEP = 2 };
    static std::array<char, 10> temp_buff; //temporary buffer to print value for text measurements
    virtual void click(IWindowMenu &window_menu) final;
    virtual Rect16 getRollingRect(IWindowMenu &window_menu, Rect16 rect) const override;
    std::array<Rect16, 2> getRollingSpinRects(IWindowMenu &window_menu, Rect16 rect) const;
    static Rect16 getSpinRect(IWindowMenu &window_menu, Rect16 base_rolling_rect, size_t spin_strlen);
    virtual char *sn_prt() const = 0;
    virtual void printText(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    IWiSpin(string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : AddSuper<IWindowMenuItem>(label, id_icon, enabled, hidden) {}
    virtual void OnClick() {}
};

//WI_SPIN
template <class T>
class WI_SPIN_t : public AddSuper<IWiSpin> {

public: //todo private
    T value;
    const T *range;
    const char *prt_format;

protected:
    virtual char *sn_prt() const;

public:
    WI_SPIN_t(T value, const T *range, const char *prt_format, string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif) override;
    void ClrVal() { value = static_cast<T>(0); }
};

//WI_SPIN_I08_t
//defines print format for int8_t version of WI_SPIN_t
class WI_SPIN_I08_t : public WI_SPIN_t<int8_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I08_t(int8_t value, const int8_t *range, string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<int8_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_I16_t
//defines print format for int16_t version of WI_SPIN_t
class WI_SPIN_I16_t : public WI_SPIN_t<int16_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I16_t(int16_t value, const int16_t *range, string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<int16_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_I32_t
//defines print format for int32_t version of WI_SPIN_t
class WI_SPIN_I32_t : public WI_SPIN_t<int32_t> {
    constexpr static const char *prt_format = "%d";

public:
    WI_SPIN_I32_t(int32_t value, const int32_t *range, string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<int32_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U08_t
//defines print format for uint8_t version of WI_SPIN_t
class WI_SPIN_U08_t : public WI_SPIN_t<uint8_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U08_t(uint8_t value, const uint8_t *range, string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<uint8_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U16_t
//defines print format for uint16_t version of WI_SPIN_t
class WI_SPIN_U16_t : public WI_SPIN_t<uint16_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U16_t(uint16_t value, const uint16_t *range, string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<uint16_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//WI_SPIN_U32_t
//defines print format for uint32_t version of WI_SPIN_t
class WI_SPIN_U32_t : public WI_SPIN_t<uint32_t> {
    constexpr static const char *prt_format = "%u";

public:
    WI_SPIN_U32_t(uint32_t value, const uint32_t *range, string_view_utf8 label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : WI_SPIN_t<uint32_t>(value, range, prt_format, label, id_icon, enabled, hidden) {}
};

//todo inherit from WI_SPIN_t<const char**>
class IWiSwitch : public AddSuper<IWindowMenuItem> {
public:
    size_t index; //todo private
    IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, bool enabled, bool hidden);
    virtual void ClrIndex() { index = 0; }
    virtual size_t size() = 0;
    virtual bool Change(int dif);
    virtual bool SetIndex(size_t idx);

protected:
    virtual void OnChange(size_t old_index) = 0;
    virtual void click(IWindowMenu &window_menu) final;
    Rect16 getSpinRect(IWindowMenu &window_menu, Rect16 base_rolling_rect, size_t spin_strlen) const;
    std::array<Rect16, 2> getRollingSpinRects(IWindowMenu &window_menu, Rect16 base_rolling_rect) const;
    virtual Rect16 getRollingRect(IWindowMenu &window_menu, Rect16 rect) const override;
    virtual void printText(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;
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
    WI_SWITCH_t(int32_t index, string_view_utf8 label, uint16_t id_icon, bool enabled, bool hidden, E &&... e)
        : AddSuper<IWiSwitch>(index, label, id_icon, enabled, hidden)
        , items { { std::forward<E>(e)... } } {}

protected:
    virtual const char *get_item() const override { return items[index]; }
};

//most common version of WI_SWITCH with on/off options
class WI_SWITCH_OFF_ON_t : public WI_SWITCH_t<2> {
    constexpr static const char *str_Off = N_("Off");
    constexpr static const char *str_On = N_("On");

public:
    WI_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, uint16_t id_icon, bool enabled, bool hidden)
        : WI_SWITCH_t<2>(size_t(index), label, id_icon, enabled, hidden, str_Off, str_On) {}
};

//currently broken todo FIXME
//WI_SELECT == switch with no label
//but can be selected like WI_SPIN
class WI_SELECT_t : public IWindowMenuItem {
public: //todo private
    uint32_t index;
    const char **strings;

protected:
    virtual void printText(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const override;

public:
    WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif) override;
};

/*****************************************************************************/
//template definitions
//WI_SPIN_t
template <class T>
WI_SPIN_t<T>::WI_SPIN_t(T value, const T *range, const char *prt_format, string_view_utf8 label, uint16_t id_icon, bool enabled, bool hidden)
    : AddSuper<IWiSpin>(label, id_icon, enabled, hidden)
    , value(value)
    , range(range)
    , prt_format(prt_format) {}

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
