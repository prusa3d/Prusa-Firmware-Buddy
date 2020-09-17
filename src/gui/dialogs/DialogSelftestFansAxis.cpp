#include "DialogSelftestFansAxis.hpp"
#include "gui.hpp" //resource_font
#include "i18n.h"
#include "wizard_config.h"

static constexpr size_t col_0 = WIZARD_MARGIN_LEFT;
static constexpr size_t col_0_w = 200;
static constexpr size_t col_1 = col_0_w + WIZARD_MARGIN_LEFT;

DialogSelftestFansAxis::DialogSelftestFansAxis()
    : IDialogMarlin()
    , text_fan_test(this, Rect16(col_0, 40, WIZARD_X_SPACE, 22), is_multiline::no, is_closed_on_click_t::no, _("Fan test"))
    , progress_fan(this, Rect16(col_0, 62, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK)
    , text_extruder_fan(this, Rect16(col_0, 74, col_1, 22), is_multiline::no, is_closed_on_click_t::no, _("Hotend fan"))
    , icon_extruder_fan(this, Rect16(col_0, 62, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK)
    , text_print_fan;
, icon_print_fan;
, text_checking_axis;
, progress_axis(this, Rect16(col_0, 62, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK), text_x_axis;
, icon_x_axis;
, text_y_axis;
, icon_y_axis;
, text_z_axis;
, icon_z_axis;
{
}
