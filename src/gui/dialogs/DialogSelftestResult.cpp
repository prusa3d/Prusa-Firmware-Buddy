/**
 * @file DialogSelftestResult.cpp
 * @author Radek Vana
 * @date 2020-12-05
 */

#include "DialogSelftestResult.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "display.h"
#include "ScreenHandler.hpp"

static constexpr size_t col_0 = WIZARD_MARGIN_LEFT;
static constexpr size_t col_0_w = 200;
static constexpr size_t col_1 = col_0_w + WIZARD_MARGIN_LEFT;

static constexpr size_t txt_h = 22;
static constexpr size_t row_h = 24;

static constexpr size_t line_width = 1;
static constexpr size_t line_space = 3;

static constexpr size_t row_fan_0 = 40;
static constexpr size_t row_fan_line = row_fan_0 + row_h;
static constexpr size_t row_fan_1 = row_fan_line + line_width + line_space;
static constexpr size_t row_fan_2 = row_fan_1 + row_h;

static constexpr size_t row_axis_0 = row_fan_2 + row_h;
static constexpr size_t row_axis_line = row_axis_0 + row_h;
static constexpr size_t row_axis_1 = row_axis_line + line_width + line_space;
static constexpr size_t row_axis_2 = row_axis_1 + row_h;
static constexpr size_t row_axis_3 = row_axis_2 + row_h;

static constexpr size_t row_temp_0 = row_axis_3 + row_h;
static constexpr size_t row_temp_line = row_temp_0 + row_h;
static constexpr size_t row_temp_1 = row_temp_line + line_width + line_space;
static constexpr size_t row_temp_2 = row_temp_1 + row_h;

static constexpr const char *txt_en_fan_test = "Fan Test";
static constexpr const char *txt_en_hotend_fan = "Hotend Fan";
static constexpr const char *txt_en_print_fan = "Print Fan";
static constexpr const char *txt_en_checking_axis = "Checking axes";
static constexpr const char *txt_en_x_axis = "X-axis";
static constexpr const char *txt_en_y_axis = "Y-axis";
static constexpr const char *txt_en_z_axis = "Z-axis";
static constexpr const char *txt_en_checking_temp = "Nozzle and Heatbed";
static constexpr const char *txt_en_noz = "Nozzle";
static constexpr const char *txt_en_bed = "Heatbed";

DialogSelftestResult::DialogSelftestResult(SelftestResultEEprom_t result)
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreenBodyNoFoot)
    //fans
    , text_fan_test(this, Rect16(col_0, row_fan_0, WIZARD_X_SPACE, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_fan_test))
    , text_hotend_fan(this, Rect16(col_0, row_fan_1, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_hotend_fan))
    , icon_hotend_fan(this, { col_1, row_fan_1 }, SelftestStateFromEeprom(result.fan0))
    , text_print_fan(this, Rect16(col_0, row_fan_2, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_print_fan))
    , icon_print_fan(this, { col_1, row_fan_2 }, SelftestStateFromEeprom(result.fan1))
    //axis
    , text_checking_axis(this, Rect16(col_0, row_axis_0, WIZARD_X_SPACE, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_checking_axis))
    , text_x_axis(this, Rect16(col_0, row_axis_1, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_x_axis))
    , icon_x_axis(this, { col_1, row_axis_1 }, SelftestStateFromEeprom(result.xaxis))
    , text_y_axis(this, Rect16(col_0, row_axis_2, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_y_axis))
    , icon_y_axis(this, { col_1, row_axis_2 }, SelftestStateFromEeprom(result.yaxis))
    , text_z_axis(this, Rect16(col_0, row_axis_3, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_z_axis))
    , icon_z_axis(this, { col_1, row_axis_3 }, SelftestStateFromEeprom(result.zaxis))
    //temp
    , text_checking_temp(this, Rect16(col_0, row_temp_0, WIZARD_X_SPACE, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_checking_temp))
    , text_noz(this, Rect16(col_0, row_temp_1, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_noz))
    , icon_noz(this, { col_1, row_temp_1 }, SelftestStateFromEeprom(result.nozzle))
    , text_bed(this, Rect16(col_0, row_temp_2, col_0_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(txt_en_bed))
    , icon_bed(this, { col_1, row_temp_2 }, SelftestStateFromEeprom(result.bed)) {
}

void DialogSelftestResult::unconditionalDraw() {
    super::unconditionalDraw();
    display::DrawRect(Rect16(col_0, row_fan_line, WIZARD_X_SPACE, 1), COLOR_SILVER);
    display::DrawRect(Rect16(col_0, row_axis_line, WIZARD_X_SPACE, 1), COLOR_SILVER);
    display::DrawRect(Rect16(col_0, row_temp_line, WIZARD_X_SPACE, 1), COLOR_SILVER);
}

void DialogSelftestResult::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        Screens::Access()->Close(); // MakeBlocking consumes close
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

//static
void DialogSelftestResult::Show(SelftestResultEEprom_t result) {
    DialogSelftestResult dlg(result);
    dlg.MakeBlocking();
}
