// screen_test_wizard_icons.cpp

#include "screen_test_wizard_icons.hpp"
#include "config.h"
#include "i18n.h"

static const char label_wizard_icon_na[] = N_("icon_na");
static const char label_wizard_icon_ok[] = N_("icon_ok");
static const char label_wizard_icon_ng[] = N_("icon_ng");
static const char label_wizard_icon_ip0[] = N_("icon_ip0");
static const char label_wizard_icon_ip1[] = N_("icon_ip1");
static const char label_wizard_icon_hourglass[] = N_("icon_hourglass");
static const char label_wizard_icon_autohome[] = N_("icon_autohome");
static const char label_wizard_icon_search[] = N_("icon_search");
static const char label_wizard_icon_measure[] = N_("icon_measure");

screen_test_wizard_icons::screen_test_wizard_icons()
    : window_frame_t()
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

    , ico_na(this, Rect16(220 - 22, 76, 22, 22), IDR_PNG_wizard_icon_na)
    , ico_ok(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_ok)
    , ico_ng(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_ng)
    , ico_ip0(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_ip0)
    , ico_ip1(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_ip1)
    , ico_hourglass(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_hourglass)
    , ico_autohome(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_autohome)
    , ico_search(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_search)
    , ico_measure(this, this->GenerateRect(ShiftDir_t::Bottom), IDR_PNG_wizard_icon_measure)

{
    static const char tm[] = N_("TEST Wizard ico");
    tst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tm));

    static const char bck[] = N_("back");
    back.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)bck));
}
