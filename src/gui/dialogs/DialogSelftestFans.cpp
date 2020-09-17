#include "DialogSelftestFans.hpp"
#include "i18n.h"
#include "wizard_config.h"

static constexpr size_t col_0 = WIZARD_MARGIN_LEFT;
static constexpr size_t col_0_w = 200;
static constexpr size_t col_1 = col_0_w + WIZARD_MARGIN_LEFT;

DialogSelftestFans::DialogSelftestFans()
    : IDialogMarlin()
    , text_fan_test(this, Rect16(col_0, 40, WIZARD_X_SPACE, 22), is_multiline::no, is_closed_on_click_t::no, _("Fan test"))
    , progress_fan(this, Rect16(col_0, 62, WIZARD_X_SPACE, 8), COLOR_LIME, COLOR_BLACK)
    , text_extruder_fan(this, Rect16(col_0, 74, col_0_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Hotend fan"))
    , icon_extruder_fan(this, { col_1, 74 })
    , text_print_fan(this, Rect16(col_0, 96, col_0_w, 22), is_multiline::no, is_closed_on_click_t::no, _("Print fan"))
    , icon_print_fan(this, { col_1, 96 }) {
}
