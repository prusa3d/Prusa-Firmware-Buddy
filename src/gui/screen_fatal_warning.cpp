#include "screen_fatal_warning.hpp"
#include "img_resources.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "display.h"
#include "sound.hpp"
#include "sys.h"
#include "support_utils.h"

#include <stdlib.h>
#include <crash_dump/dump.hpp>
#include <error_codes.hpp>
#include <config_store/store_instance.hpp>

using namespace crash_dump;

static const constexpr Rect16 warning_rect = Rect16(30, 85, 48, 48);
static const constexpr Rect16 QR_rect = Rect16(320, 40, 130, 130);
static const constexpr Rect16 hand_rect = Rect16(320 + (130 - 59) / 2, 130 + 40 + 5, 59, 72);
static const constexpr Rect16 descr_rect = Rect16(30 + 48 + 10, 85, 215, 100);
static const constexpr Rect16 link_rect = Rect16(30 + 48 + 10, 222, 215, 20);
static const constexpr Rect16 qr_code_rect = Rect16(180, 265, 100, 20);
static const constexpr Rect16 help_txt_rect = Rect16(30 + 48 + 10, 200, 215, 20);

static constexpr const char *const header_label = N_("Wrong hardware");
static constexpr const char *const help_text = N_("More detail at");
static constexpr const char *const unknown_err_txt = N_("Unknown Error");

ScreenFatalWarning::ScreenFatalWarning()
    : AddSuperWindow<ScreenResetError>()
    , header(this)
    , err_title(this, title_rect, is_multiline::no)
    , err_description(this, descr_rect, is_multiline::yes)
    , hand_icon(this, hand_rect, &img::hand_qr_59x72)
    , warning_icon(this, warning_rect, &img::warning_48x48)
    , qr(this, QR_rect, 1, Align_t::RightTop()) // error code is passed in the constructor
    , help_txt(this, help_txt_rect, is_multiline::no)
    , help_link(this, link_rect, is_multiline::no)
    , qr_code_txt(this, qr_code_rect, is_multiline::no)
    , anim(Animator_LCD_leds().start_animations(Fading(leds::Color(255, 0, 0), 500), 10)) {

    img::enable_resource_file();

    help_link.set_font(resource_font(IDR_FNT_SMALL));
    qr_code_txt.set_font(resource_font(IDR_FNT_SMALL));
    err_title.SetAlignment(Align_t::LeftTop());

    help_link.SetAlignment(Align_t::LeftTop());
    qr_code_txt.SetAlignment(Align_t::CenterTop());

    header.SetIcon(&img::error_16x16);
    header.SetText(_(header_label));

    // Extract error code from xflash
    uint16_t error_code = load_message_error_code(); // Unknow code == ERR_UNDEF == 0

    const auto show_qr = [&]() {
        qr.SetQRHeader(error_code);
        /// draw short URL
        const char *qr_text = qr.GetQRShortText();
        help_txt.SetText(_(help_text));
        help_link.SetText(_(qr_text));

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
#if PRINTER_IS_PRUSA_MK4
            if (!config_store().xy_motors_400_step.get()) {
                static_assert(ERR_PRINTER_CODE == 13, "PRUSA MK4's PID is no longer 13, which means this hardcoded calculation is no longer correct.");
                error_code += 8000; // If MK4 has 200 step motors, it means it is MK3.9, which has it's own product ID (21 instead of MK4's 13) - so 13XXX have to be change in runtime to 21XXX (+8000)
            }
#endif
            show_qr();
        }
    }
}

void ScreenFatalWarning::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if ((event == GUI_event_t::CLICK) || (event == GUI_event_t::BTN_DN)) {
        sys_reset();
        return;
    }
    SuperWindowEvent(sender, event, param);
}
