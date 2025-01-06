#include "screen_qr_error.hpp"
#include "img_resources.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "sys.h"
#include "support_utils.h"

#include <stdlib.h>
#include <crash_dump/dump.hpp>
#include <error_codes.hpp>
#include <error_code_mangle.hpp>
#include <config_store/store_instance.hpp>
#include <guiconfig/guiconfig.h>
#include <option/has_leds.h>

using namespace crash_dump;

static const constexpr Rect16 hand_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(270, 95, 60, 82) : Rect16(20, 155, 64, 82);
static const constexpr Rect16 descr_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 85, 230, 170) : Rect16(10, 50, GuiDefaults::ScreenWidth - 20, 220);
static const constexpr Rect16 QR_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(320, 85, 130, 130) : Rect16(90, 140, 130, 130);
static const constexpr Rect16 link_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 277, 170, 20) : Rect16(0, 270, GuiDefaults::ScreenWidth, 13);
static const constexpr Rect16 qr_code_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(320, 257, 130, 20) : Rect16(100, 295, 64, 13);
static const constexpr Rect16 help_txt_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 255, 210, 20) : Rect16(30, 200, 215, 20);
static const constexpr Rect16 title_line_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 70, 420, 1) : Rect16(10, 44, 219, 1);
static const constexpr Rect16 fw_version_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(200, 277, 250, 20) : Rect16(6, 295, GuiDefaults::ScreenWidth - 6, 13);
static constexpr const char *const header_label = N_("ERROR");
static constexpr const char *const help_text = N_("More detail at");
static constexpr const char *const unknown_err_txt = N_("Unknown Error");

ScreenErrorQR::ScreenErrorQR()
    : ScreenResetError(fw_version_rect)
    , header(this)
    , err_title(this, title_rect, is_multiline::no)
    , err_description(this, descr_rect, is_multiline::yes)
    , hand_icon(this, hand_rect, &img::hand_qr_59x72)
    , qr(this, QR_rect, ErrCode::ERR_UNDEF)
    , help_txt(this, help_txt_rect, is_multiline::no)
    , help_link(this, link_rect, ErrCode::ERR_UNDEF)
    , qr_code_txt(this, qr_code_rect, is_multiline::no)
#if HAS_LEDS()
    , anim(Animator_LCD_leds().start_animations(Fading(leds::ColorRGBW(255, 0, 0), 500), 10))
#endif
    , title_line(this, title_line_rect) {

    img::enable_resource_file();
    SetRedLayout();
    title_line.SetBackColor(COLOR_WHITE);
    help_link.set_font(Font::small);
    qr_code_txt.set_font(Font::small);
#if HAS_MINI_DISPLAY()
    err_title.set_font(Font::small);
    err_title.SetAlignment(Align_t::LeftBottom());
    err_description.set_font(Font::small);
    qr_code_txt.SetAlignment(Align_t::CenterTop());
#elif HAS_LARGE_DISPLAY()
    err_title.SetAlignment(Align_t::LeftTop());
    qr_code_txt.SetAlignment(Align_t::Right());
    qr.SetAlignment(Align_t::Right());
    fw_version_txt.SetAlignment(Align_t::Right());
#endif

    hand_icon.SetAlignment(Align_t::Center());
    help_link.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::LeftTop() : Align_t::CenterTop());

    header.SetIcon(&img::error_16x16);
    header.SetText(_(header_label));

    // Extract error code from xflash
    uint16_t error_code = load_message_error_code(); // Unknow code == ERR_UNDEF == 0

    const auto show_qr = [&]() {
        qr.set_error_code(ErrCode(error_code));
        if (GuiDefaults::EnableDialogBigLayout) {
            help_txt.SetText(_(help_text));
        } else {
            help_txt.Hide();
        }
        help_link.set_error_code(ErrCode(error_code));

        if (config_store().devhash_in_qr.get()) {
            static char p_code[PRINTER_CODE_SIZE + 1];
            printerCode(p_code);
            qr_code_txt.SetText(string_view_utf8::MakeRAM((const uint8_t *)p_code));
        } else {
            qr_code_txt.Hide();
        }
    };

    const auto hide_qr = [&]() {
        help_txt.Hide();
        help_link.Hide();
        hand_icon.Hide();
        qr.Hide();
        qr_code_txt.Hide();
    };

    static char err_title_buff[MSG_TITLE_MAX_LEN] {};
    static char err_message_buff[MSG_MAX_LEN] {};
    if (message_get_type() == MsgType::RSOD && load_message(err_message_buff, MSG_MAX_LEN, err_title_buff, MSG_TITLE_MAX_LEN)) {
        err_title.SetText(err_title_buff[0] ? _(err_title_buff) : _(unknown_err_txt));
        err_description.SetText(_(err_message_buff));

        if (error_code != static_cast<std::underlying_type_t<ErrCode>>(ErrCode::ERR_UNDEF) && error_code / 1000 == ERR_PRINTER_CODE) {
            show_qr();
        } else {
            hide_qr();
        }
    } else {
        err_title.SetText(_(unknown_err_txt));
        err_description.Hide();
        title_line.Hide();
        hide_qr();
    }
}

void ScreenErrorQR::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if ((event == GUI_event_t::CLICK) || (event == GUI_event_t::BTN_DN)) {
        sys_reset();
        return;
    }
    ScreenResetError::windowEvent(sender, event, param);
}
