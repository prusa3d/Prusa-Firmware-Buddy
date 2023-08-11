// screen_test_wizard_icons.cpp

#include "screen_test_wizard_icons.hpp"
#include "img_resources.hpp"
#include "config.h"
#include "i18n.h"

// testing code - intentionally not translated
static const char label_wizard_icon_na[] = "icon_na";
static const char label_wizard_icon_ok[] = "icon_ok";
static const char label_wizard_icon_ng[] = "icon_ng";
static const char label_wizard_icon_ip0[] = "icon_ip0";
static const char label_wizard_icon_ip1[] = "icon_ip1";
static const char label_wizard_icon_hourglass[] = "icon_hourglass";
static const char label_wizard_icon_autohome[] = "icon_autohome";
static const char label_wizard_icon_search[] = "icon_search";
static const char label_wizard_icon_measure[] = "icon_measure";
static const char wizard_icon_autohome_resource_addr[] = "/internal/res/png/wizard_icon_autohome.png";
static const char wizard_icon_search_resource_addr[] = "/internal/res/png/wizard_icon_search.png";
static const char wizard_icon_measure_resource_addr[] = "/internal/res/png/wizard_icon_measure.png";

screen_test_wizard_icons::screen_test_wizard_icons()
    : AddSuperWindow<screen_t>()
    , tst(this, Rect16(10, 32, 220, 22), is_multiline::no)
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes)
    , txt_na(this, Rect16(10, 76, 220 - 22, 22), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_na))
    , txt_ok(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_ok))
    , txt_ng(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_ng))
    , txt_ip0(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_ip0))
    , txt_ip1(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_ip1))
    , txt_hourglass(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_hourglass))

    , ico_na(this, Rect16(220 - 22, 76, 22, 22), &img::dash_18x18)
    , ico_ok(this, this->GenerateRect(ShiftDir_t::Bottom), &img::ok_color_18x18)
    , ico_ng(this, this->GenerateRect(ShiftDir_t::Bottom), &img::nok_color_18x18)
    , ico_ip0(this, this->GenerateRect(ShiftDir_t::Bottom), &img::spinner0_16x16)
    , ico_ip1(this, this->GenerateRect(ShiftDir_t::Bottom), &img::spinner1_16x16)
    , ico_hourglass(this, this->GenerateRect(ShiftDir_t::Bottom), &img::hourglass_26x39)

{
    // testing code - intentionally not translated
    static const char tm[] = "TEST Wizard ico";
    tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tm));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));
}
