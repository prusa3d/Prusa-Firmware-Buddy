// screen_menu_odometer.cpp

#include <stdlib.h>

#include "cmath_ext.h"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "i18n.h"
#include "odometer.hpp"

using MenuContainer = WinMenuContainer<MI_RETURN>;

class ScreenMenuOdometer : public AddSuperWindow<screen_t> {
    static constexpr const char *label = N_("ODOMETER");
    static const constexpr char *x_text = N_("X axis");
    static const constexpr char *y_text = N_("Y axis");
    static const constexpr char *z_text = N_("Z axis");
    static const constexpr char *e_text = N_("Filament");
    static const constexpr char *val_format = "%0.1f m"; //do not translate

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t x_txt;
    window_text_t y_txt;
    window_text_t z_txt;
    window_text_t e_txt;
    window_numb_t x_val;
    window_numb_t y_val;
    window_numb_t z_val;
    window_numb_t e_val;

    static float getVal(int index) {
        odometer_s.force_to_eeprom();
        return odometer_s.get(index) * .001f;
    }

public:
    ScreenMenuOdometer();
};

ScreenMenuOdometer::ScreenMenuOdometer()
    : AddSuperWindow<screen_t>(nullptr, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, Rect16(GuiDefaults::RectScreenBody) = Rect16::Height_t(30), &container)
    , header(this)
    , x_txt(this, Rect16(menu.Left(), menu.GetRect().EndPoint().y, menu.Width() / 2, resource_font(IDR_FNT_BIG)->h), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(x_text)))
    , y_txt(this, Rect16(x_txt.GetRect(), ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(y_text)))
    , z_txt(this, Rect16(y_txt.GetRect(), ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(z_text)))
    , e_txt(this, Rect16(z_txt.GetRect(), ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(e_text)))
    , x_val(this, Rect16(x_txt.GetRect(), ShiftDir_t::Right), getVal(0), val_format)
    , y_val(this, Rect16(x_val.GetRect(), ShiftDir_t::Bottom), getVal(1), val_format)
    , z_val(this, Rect16(y_val.GetRect(), ShiftDir_t::Bottom), getVal(2), val_format)
    , e_val(this, Rect16(z_val.GetRect(), ShiftDir_t::Bottom), getVal(3), val_format) {
    header.SetText(_(label));
    header.SetIcon(IDR_PNG_info_16px);

    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list
}

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenMenuOdometer>();
}
