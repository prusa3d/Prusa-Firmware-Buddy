// screen_menu_odometer.cpp

#include <stdlib.h>

#include "cmath_ext.h"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "i18n.h"
#include "odometer.hpp"
#include "window_numb.hpp"

using MenuContainer = WinMenuContainer<MI_RETURN>;

class ScreenMenuOdometer : public AddSuperWindow<screen_t> {
    static constexpr const char *label = N_("STATISTICS");
    static const constexpr char *x_text = N_("X-axis");
    static const constexpr char *y_text = N_("Y-axis");
    static const constexpr char *z_text = N_("Z-axis");
    static const constexpr char *e_text = N_("Filament");
    static const constexpr char *t_text = N_("Print Time");
    static const constexpr char *val_format = "%0.1f m"; // do not translate
    static const constexpr char *time_format = "%u s";   // do not translate

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t x_txt;
    window_text_t y_txt;
    window_text_t z_txt;
    window_text_t e_txt;
    window_text_t t_txt;
    window_numb_t x_val;
    window_numb_t y_val;
    window_numb_t z_val;
    window_numb_t e_val;
    window_numb_t t_val;

    static float getVal(Odometer_s::axis_t axis) {
        Odometer_s::instance().force_to_eeprom();
        return Odometer_s::instance().get(axis) * .001f;
    }
    static uint32_t getTime() {
        Odometer_s::instance().force_to_eeprom();
        return Odometer_s::instance().get_time();
    }

public:
    ScreenMenuOdometer();
};

ScreenMenuOdometer::ScreenMenuOdometer()
    : AddSuperWindow<screen_t>(nullptr, GuiDefaults::RectScreen, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, Rect16(GuiDefaults::RectScreenBody) = Rect16::Height_t(30), &container)
    , header(this)
    , x_txt(this, Rect16(menu.rect.Left(), menu.rect.EndPoint().y, menu.rect.Width() / 2, resource_font(IDR_FNT_BIG)->h), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(x_text)))
    , y_txt(this, Rect16(x_txt.rect, ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(y_text)))
    , z_txt(this, Rect16(y_txt.rect, ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(z_text)))
    , e_txt(this, Rect16(z_txt.rect, ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(e_text)))
    , t_txt(this, Rect16(e_txt.rect, ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8(_(t_text)))
    , x_val(this, Rect16(x_txt.rect, ShiftDir_t::Right), getVal(Odometer_s::axis_t::X), val_format)
    , y_val(this, Rect16(x_val.rect, ShiftDir_t::Bottom), getVal(Odometer_s::axis_t::Y), val_format)
    , z_val(this, Rect16(y_val.rect, ShiftDir_t::Bottom), getVal(Odometer_s::axis_t::Z), val_format)
    , e_val(this, Rect16(z_val.rect, ShiftDir_t::Bottom), getVal(Odometer_s::axis_t::E), val_format)
    , t_val(this, Rect16(e_val.rect, ShiftDir_t::Bottom), (float)getTime(), time_format) {

    t_val.PrintAsTime();

    header.SetText(_(label));
    header.SetIcon(IDR_PNG_info_16px);

    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list
}

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenMenuOdometer>();
}
