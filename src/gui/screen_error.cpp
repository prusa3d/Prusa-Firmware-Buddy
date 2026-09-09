#include <screen_error.hpp>

#include <common/printer_model.hpp>
#include <crash_dump/dump.hpp>
#include <error_codes.hpp>
#include <sound.hpp>
#include <version/version.hpp>
#include <utils/string_builder.hpp>
#include <bsod_gui.hpp>
#include <img_resources.hpp>
#include <find_error.hpp>

#include <option/has_leds.h>
#if HAS_LEDS()
    #include <leds/status_leds_handler.hpp>
#endif

using namespace crash_dump;
using namespace bsod_details;

static const constexpr Rect16 QR_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(350, 85, 100, 100) : Rect16(150, 190, 80, 80);
static const constexpr Rect16 link_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 270, 200, 20) : Rect16(10, 225, 130, 13);
static const constexpr Rect16 printer_model_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(210, 270, 240, 20) : Rect16(10, 267, 220, 13);
static const constexpr Rect16 help_txt_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 250, 170, 20) : Rect16(10, 210, 160, 13);
static const constexpr Rect16 title_line_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 70, 420, 1) : Rect16(10, 44, 220, 1);
static const constexpr Rect16 fw_version_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(210, 250, 240, 20) : Rect16(10, 300, 220, 13);
static const constexpr Rect16 scan_me_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(350, 190, 100, 20) : Rect16(160, 267, 70, 13);
static const constexpr Rect16 debug_info_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, 290, 420, 20) : Rect16(10, 285, 220, 13);
static const constexpr Rect16 bsod_msg_rect = GuiDefaults::EnableDialogBigLayout ? Rect16(30, GuiDefaults::RedscreenDescriptionRect.Top(), GuiDefaults::RedscreenDescriptionRect.Width(), 40) : Rect16(10, GuiDefaults::RedscreenDescriptionRect.Top(), GuiDefaults::RedscreenDescriptionRect.Width(), 26);

static constexpr const char *const txt_error = N_("ERROR");
static constexpr const char *const txt_help = N_("More details at");
static constexpr const char *const txt_scanme = N_("Scan me");

ScreenError::ScreenError()
    : screen_t(nullptr, win_type_t::normal, is_closed_on_timeout_t::no, is_closed_on_printing_t::no)
    , header(this, _(txt_error))
    , txt_err_title(this, GuiDefaults::RedscreenTitleRect, is_multiline::no)
    , txt_err_desc(this, GuiDefaults::RedscreenDescriptionRect, is_multiline::yes)
    , txt_printer_model(this, printer_model_rect, is_multiline::no)
    , txt_fw_version(this, fw_version_rect, is_multiline::no)
    , txt_helper(this, help_txt_rect, is_multiline::no, is_closed_on_click_t::no, _(txt_help))
    , txt_scan_me(this, scan_me_rect, is_multiline::no, is_closed_on_click_t::no, _(txt_scanme))
    , txt_debug_info(this, debug_info_rect, is_multiline::no)
    , txt_bsod_msg(this, bsod_msg_rect, is_multiline::yes)
    , txt_help_link(this, link_rect, ErrCode::ERR_UNDEF)
    , qr(this, QR_rect, ErrCode::ERR_UNDEF)
    , title_line(this, title_line_rect) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();

    header.SetIcon(&img::error_16x16);

    sound::stop();
    sound::play(SoundType::critical_alert);

    if constexpr (!GuiDefaults::EnableDialogBigLayout) {
        txt_err_desc.set_font(Font::small);
        txt_scan_me.set_font(Font::small);
        txt_bsod_msg.set_font(Font::small);
    }
    txt_fw_version.set_font(Font::small);
    txt_help_link.set_font(Font::small);
    txt_printer_model.set_font(Font::small);
    txt_helper.set_font(Font::small);
    txt_debug_info.set_font(Font::small);
    qr.SetAlignment(Align_t::Right());
    txt_scan_me.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::Center() : Align_t::Center());
    txt_fw_version.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::Right() : Align_t::Left());
    txt_printer_model.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::Right() : Align_t::Left());
    txt_debug_info.SetAlignment(GuiDefaults::EnableDialogBigLayout ? Align_t::Right() : Align_t::Left());
    txt_bsod_msg.set_check_overflow(false);
    txt_fw_version.set_check_overflow(false); // versions with git tag will probably fail the asserts
    txt_fw_version.SetText(string_view_utf8::MakeCPUFLASH(version::project_version_full));
    txt_printer_model.SetText(string_view_utf8::MakeCPUFLASH(PrinterModelInfo::current().display_str()));

#if HAS_LEDS()
    leds::StatusLedsHandler::instance().set_error();
#endif

    // Extract error from xflash
    uint16_t error_code = load_message_error_code(); // Unknow code == ERR_UNDEF == 0
    auto msg_type = message_get_type();
    const auto &internal_error = find_error(ErrCode::ERR_SYSTEM_INTERNAL_ERROR);

    // Extract crash dump info
    const bool msg_successfuly_loaded = load_message(err_msg_buff.data(), MSG_MAX_LEN, err_title_buff.data(), MSG_TITLE_MAX_LEN);
    // Function fatal_error can create message for RSOD with custom message, but ErrCode::ErrUndef, so the function
    // is set to also handle messages with non-empty title or message. All-zero results in default error message.
    const bool show_something_specific = (((error_code / 1000) == ERR_PRINTER_CODE)
        || (err_title_buff.data()[0] != '\0')
        || (err_msg_buff.data()[0] != '\0'));

    if ((!msg_successfuly_loaded) || (!show_something_specific)) {
        // Fallback to default error message
        error_code = static_cast<uint16_t>(internal_error.err_code);
        txt_err_title.SetText(_(internal_error.err_title));
        txt_err_desc.SetText(_(internal_error.err_text));
        txt_debug_info.Hide();
        msg_type = MsgType::EMPTY; // Extracting crash dump was not successful, show generic BSOD
    } else {
        // Parse debug info & set error screen layout
        switch (msg_type) {
        case MsgType::RSOD:
            SetRedLayout();
            title_line.SetBackColor(COLOR_WHITE);
            txt_debug_info.Hide();
            break;
        case MsgType::IWDGW: {
            static constexpr const char *iwdgw_prefix = "IWDGW ";
            const auto prefix_len = strlen(iwdgw_prefix);
            StringBuilder sb(debug_info_buff);
            sb.append_string(iwdgw_prefix);
            get_task_name(debug_info_buff.data() + prefix_len, std::size(debug_info_buff) - prefix_len);
        } break;
        case MsgType::BSOD: {
            StringBuilder sb(debug_info_buff);
            sb.append_printf("BSOD %s", err_title_buff.data());

            auto rect = txt_err_desc.GetRect();
            rect += Rect16::Top_t(bsod_msg_rect.Height());
            rect -= Rect16::Height_t(bsod_msg_rect.Height());
            txt_err_desc.SetRect(rect);
            txt_bsod_msg.SetText(string_view_utf8::MakeRAM(err_msg_buff.data()));

        } break;
        case MsgType::STACK_OVF: {
            StringBuilder sb(debug_info_buff);
            sb.append_printf("STACK_OVF %s", err_title_buff.data());
        } break;
        case MsgType::FATAL_WARNING:
            txt_debug_info.Hide();
            break;
        case MsgType::FACTORY_RESET:
            [[fallthrough]];
        case MsgType::EMPTY:
            break;
        }

        if (msg_type == MsgType::RSOD || msg_type == MsgType::FATAL_WARNING) {
            txt_err_title.SetText(_(err_title_buff.data()));
            txt_err_desc.SetText(_(err_msg_buff.data()));
        } else {
            txt_err_title.SetText(_(internal_error.err_title));
            txt_err_desc.SetText(_(internal_error.err_text));
        }
        txt_debug_info.SetText(string_view_utf8::MakeRAM(debug_info_buff.data()));
    }

    if (msg_type != MsgType::BSOD) {
        txt_bsod_msg.Hide();
    }
    if (msg_type != MsgType::RSOD) {
        // Set up Black error layout
        txt_debug_info.SetTextColor(COLOR_GRAY);
        txt_printer_model.SetTextColor(COLOR_GRAY);
        txt_fw_version.SetTextColor(COLOR_GRAY);
        txt_err_title.SetTextColor(COLOR_BRAND);
        title_line.SetBackColor(COLOR_BRAND); // Have to be called after setting up layout
    }

    qr.set_error_code(ErrCode(error_code));
    txt_help_link.set_error_code(ErrCode(error_code));
}

void ScreenError::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {
    case GUI_event_t::KNOB:
    case GUI_event_t::HOLD:
    case GUI_event_t::CLICK:
        sound::stop();
        break;
    default:
        break;
    }
}
