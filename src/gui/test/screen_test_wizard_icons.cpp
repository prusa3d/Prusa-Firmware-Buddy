// screen_test_wizard_icons.cpp

#include "screen_test_wizard_icons.hpp"
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
    , txt_autohome(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_autohome))
    , txt_search(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_search))
    , txt_measure(this, this->GenerateRect(ShiftDir_t::Bottom), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)label_wizard_icon_measure))

    , ico_na(this, Rect16(220 - 22, 76, 22, 22), IDR_PNG_dash_18px)
    , ico_ok(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_ok_color_18px)
    , ico_ng(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_nok_color_18px)
    , ico_ip0(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_spinner1_16px)
    , ico_ip1(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_spinner2_16px)
    , ico_hourglass(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_hourglass_39px)
    , ico_autohome(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_autohome)
    , ico_search(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_search)
    , ico_measure(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_measure)

{
    // testing code - intentionally not translated
    static const char tm[] = "TEST Wizard ico";
    tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tm));

    static const char bck[] = "back";
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));
}
