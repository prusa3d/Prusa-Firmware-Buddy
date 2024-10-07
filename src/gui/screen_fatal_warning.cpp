#include "screen_fatal_warning.hpp"
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

using namespace crash_dump;

static const constexpr uint16_t left_padding = ScreenFatalWarning::title_rect.Left();
static const constexpr uint16_t text_start_y = 85;
static const constexpr uint16_t qr_start_x = (GuiDefaults::ScreenWidth * 2) / 3;
static const constexpr uint16_t info_text_width = 293;

static const constexpr Rect16 QR_rect = Rect16(qr_start_x, 74, GuiDefaults::QRSize, GuiDefaults::QRSize);
static const constexpr Rect16 hand_rect = Rect16(qr_start_x + /* center under qr */ (GuiDefaults::QRSize - 59) / 2, GuiDefaults::QRSize + QR_rect.Top() + 5 /* visual delimeter */, 59, 72);
static const constexpr Rect16 descr_rect = Rect16(left_padding, text_start_y, info_text_width, 155);
static const constexpr Rect16 help_txt_rect = Rect16(left_padding, 242, info_text_width, 20);
static const constexpr Rect16 link_rect = Rect16(left_padding, 264, info_text_width, 20);
static const constexpr Rect16 qr_code_rect = Rect16(180, 295, 100, 20);
static const constexpr Rect16 fw_version_rect = Rect16(30, 295, GuiDefaults::ScreenWidth - 30, 20);

static constexpr const char *const header_label = N_("Wrong hardware");
static constexpr const char *const help_text = N_("More detail at");
static constexpr const char *const unknown_err_txt = N_("Unknown Error");

ScreenFatalWarning::ScreenFatalWarning()
    : ScreenResetError(fw_version_rect)
    , header(this)
    , err_title(this, title_rect, is_multiline::no)
    , err_description(this, descr_rect, is_multiline::yes)
    , hand_icon(this, hand_rect, &img::hand_qr_59x72)
    , qr(this, QR_rect, ErrCode::ERR_UNDEF)
    , help_txt(this, help_txt_rect, is_multiline::no)
    , help_link(this, link_rect, ErrCode::ERR_UNDEF)
    , qr_code_txt(this, qr_code_rect, is_multiline::no)
    , anim(Animator_LCD_leds().start_animations(Fading(leds::ColorRGBW(255, 0, 0), 500), 10)) {

    img::enable_resource_file();

    help_link.set_font(Font::small);
    qr_code_txt.set_font(Font::small);
    err_title.SetAlignment(Align_t::LeftTop());

    help_link.SetAlignment(Align_t::LeftTop());
    qr_code_txt.SetAlignment(Align_t::CenterTop());

    header.SetIcon(&img::error_16x16);
    header.SetText(_(header_label));

    // Extract error code from xflash
    uint16_t error_code = load_message_error_code(); // Unknow code == ERR_UNDEF == 0

    const auto show_qr = [&]() {
        qr.set_error_code(ErrCode(error_code));
        help_txt.SetText(_(help_text));
        help_link.set_error_code(ErrCode(error_code));

        if (config_store().devhash_in_qr.get()) {
            static char p_code[PRINTER_CODE_SIZE + 1];
            printerCode(p_code);
            qr_code_txt.SetText(string_view_utf8::MakeRAM((const uint8_t *)p_code));
        } else {
            qr_code_txt.Hide();
        }
    };

    static char err_title_buff[MSG_TITLE_MAX_LEN] {};
    static char err_message_buff[MSG_MAX_LEN] {};
    if (message_get_type() == MsgType::FATAL_WARNING && load_message(err_message_buff, MSG_MAX_LEN, err_title_buff, MSG_TITLE_MAX_LEN)) {
        err_title.SetText(err_title_buff[0] ? _(err_title_buff) : _(unknown_err_txt));
        err_description.SetText(_(err_message_buff));

        if (error_code != static_cast<std::underlying_type_t<ErrCode>>(ErrCode::ERR_UNDEF) && error_code / 1000 == ERR_PRINTER_CODE) {
            show_qr();
        }
    }
}

void ScreenFatalWarning::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if ((event == GUI_event_t::CLICK) || (event == GUI_event_t::BTN_DN)) {
        sys_reset();
        return;
    }
    ScreenResetError::windowEvent(sender, event, param);
}
