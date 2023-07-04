#include "screen_qr_error.hpp"
#include "png_resources.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "display.h"
#include "sound.hpp"
#include "sys.h"
#include "support_utils.h"
#include "version.h"

#include <stdlib.h>
#include <crash_dump/dump.hpp>
#include <error_codes.hpp>
#include <configuration_store.hpp>

using namespace crash_dump;

static const constexpr Rect16 title_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 44, display::GetW() - 60, 20) : Rect16(13, 12, display::GetW() - 26, 20);
static const constexpr Rect16 hand_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(250, 105, 65, 82) : Rect16(20, 165, 64, 82);
static const constexpr Rect16 descr_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 85, 215, 100) : Rect16(10, 42, display::GetW() - 20, 220);
static const constexpr Rect16 QR_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(320, 85, 130, 130) : Rect16(90, 130, 130, 130);
static const constexpr Rect16 link_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 222, 420, 20) : Rect16(0, 270, display::GetW(), 13);
static const constexpr Rect16 qr_code_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(180, 265, 100, 20) : Rect16(100, 295, 64, 13);
static const constexpr Rect16 fw_version_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 265, 150, 20) : Rect16(6, 295, 80, 13);
static const constexpr Rect16 signature_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(320, 265, 40, 20) : Rect16(160, 295, 40, 13);
static const constexpr Rect16 appendix_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(410, 265, 40, 20) : Rect16(195, 295, 40, 13);
static const constexpr Rect16 help_txt_rect = Rect16(30, 200, 215, 20);
static const constexpr Rect16 title_line_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 70, 420, 1) : Rect16(10, 33, 219, 1);

static constexpr const char *const header_label = N_("ERROR");
static constexpr const char *const help_text = N_("More detail at");
static constexpr const char *const unknown_err_txt = N_("Unknown Error");

ScreenErrorQR::ScreenErrorQR()
    : AddSuperWindow<screen_reset_error_data_t>()
    , header(this)
    , err_title(this, title_rect, is_multiline::no)
    , err_description(this, descr_rect, is_multiline::yes)
    , hand_icon(this, hand_rect, &png::hand_qr_59x72)
    , qr(this, QR_rect, 1, Align_t::RightTop()) // error code is passed in the constructor
    , help_txt(this, help_txt_rect, is_multiline::no)
    , help_link(this, link_rect, is_multiline::no)
    , qr_code_txt(this, qr_code_rect, is_multiline::no)
    , fw_version_txt(this, fw_version_rect, is_multiline::no)
    , signature_txt(this, signature_rect, is_multiline::no)
    , appendix_txt(this, appendix_rect, is_multiline::no)
#if HAS_LEDS
    , anim(Animator_LCD_leds().start_animations(Fading(leds::Color(255, 0, 0), 500), 10))
#endif
    , title_line(this, title_line_rect) {

    png::Resource::EnableDefaultFile();
    SetRedLayout();
    title_line.SetBackColor(COLOR_WHITE);
    help_link.set_font(resource_font(IDR_FNT_SMALL));
    qr_code_txt.set_font(resource_font(IDR_FNT_SMALL));
    fw_version_txt.set_font(resource_font(IDR_FNT_SMALL));
    signature_txt.set_font(resource_font(IDR_FNT_SMALL));
    appendix_txt.set_font(resource_font(IDR_FNT_SMALL));

    err_description.SetAlignment(Align_t::LeftTop());
    hand_icon.SetAlignment(Align_t::Center());
    help_link.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::LeftTop() : Align_t::CenterTop());
    qr_code_txt.SetAlignment(Align_t::CenterTop());
    fw_version_txt.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::LeftTop() : Align_t::CenterTop());
    signature_txt.SetAlignment(Align_t::CenterTop());
    appendix_txt.SetAlignment(Align_t::CenterTop());

    header.SetIcon(&png::error_16x16);
    header.SetText(_(header_label));

    // Extract error code from xflash
    const uint16_t error_code = dump_err_in_xflash_get_error_code(); // Unknow code == ERR_UNDEF == 0

    const auto show_qr = [&]() {
        qr.SetQRHeader(error_code);
        /// draw short URL
        const char *qr_text = qr.GetQRShortText();
        if (GuiDefaults::EnableDialogBigLayout)
            help_txt.SetText(_(help_text));
        else
            help_txt.Hide();
        help_link.SetText(_(qr_text));

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

    static char err_title_buff[DUMP_MSG_TITLE_MAX_LEN] {};
    static char err_message_buff[DUMP_MSG_MAX_LEN] {};
    if (dump_err_in_xflash_is_valid() && dump_err_in_xflash_get_message(err_message_buff, DUMP_MSG_MAX_LEN, err_title_buff, DUMP_MSG_TITLE_MAX_LEN)) {
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

    /// fw version, hash, [fw signed], [appendix]
    static const constexpr uint16_t fw_version_str_len = 13 + 1; // combined max length of project_version + .._suffix_short + null
    static char fw_version[fw_version_str_len];                  // intentionally limited to the number of practically printable characters without overwriting the nearby hash text
                                                                 // snprintf will clamp the text if the input is too long
    snprintf(fw_version, sizeof(fw_version), "%s%s", project_version, project_version_suffix_short);
    fw_version_txt.SetText(_(fw_version));

    if (signature_exist()) {
        static const char signed_fw_str[] = "[S]";
        signature_txt.SetText(_(signed_fw_str));
    } else {
        signature_txt.Hide();
    }

    if (appendix_exist()) {
        static const char appendix_str[] = "[A]";
        appendix_txt.SetText(_(appendix_str));
    } else {
        appendix_txt.Hide();
    }
}

void ScreenErrorQR::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if ((event == GUI_event_t::CLICK) || (event == GUI_event_t::BTN_DN)) {
        sys_reset();
        return;
    }
    SuperWindowEvent(sender, event, param);
}
