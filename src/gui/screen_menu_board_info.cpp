/// @file
#include "screen_menu_board_info.hpp"

#include <common/data_exchange.hpp>
#include <common/otp.hpp>
#include <device/board.h>
#include <gui/img_resources.hpp>
#include <option/has_indx_head.h>
#include <option/has_love_board.h>
#include <option/has_xbuddy_extension.h>
#include <option/has_xl_can.h>
#include <option/has_xlcd.h>
#include <utils/string_builder.hpp>

#if HAS_XBUDDY_EXTENSION()
    #include <puppies/xbuddy_extension.hpp>
#endif

#if HAS_INDX_HEAD()
    #include <puppies/INDX.hpp>
#endif

#if HAS_XL_CAN()
    #include <puppies/xl_can.hpp>
#endif

WindowMenuInfoOtpBase::WindowMenuInfoOtpBase(const OTP_v2 &otp) {
    StringBuilder { buffer }
        .append_printf("%.*s/%u", sizeof(otp.datamatrix), otp.datamatrix, otp.bomID)
        .check();
}

string_view_utf8 WindowMenuInfoOtpBase::get() const {
    return string_view_utf8::MakeRAM(buffer.data());
}

WindowMenuInfoOtp::WindowMenuInfoOtp()
    : WindowMenuInfoOtpBase { OTP_v2 {} }
    , IWiInfo {
        {},
        string_view_utf8::MakeNULLSTR(),
        nullptr,
        is_enabled_t::no,
        is_hidden_t::yes,
    } {
}

WindowMenuInfoOtp::WindowMenuInfoOtp(const char *label, const OTP_v2 &otp)
    : WindowMenuInfoOtpBase { otp }
    , IWiInfo {
        WindowMenuInfoOtpBase::get(),
        string_view_utf8::MakeCPUFLASH(label),
    } {
}

static constexpr const char *motherboard_label =
#if BOARD_IS_XBUDDY()
    "xBuddy"
#elif BOARD_IS_XLBUDDY()
    "XLBuddy"
#else
    #error
#endif
    ;

MI_OTP_MOTHERBOARD::MI_OTP_MOTHERBOARD()
    : WiInfo<28> { string_view_utf8::MakeCPUFLASH(motherboard_label) } {
    serial_nr_t serial_nr;
    otp_get_serial_nr(serial_nr);
    uint8_t bom_id = otp_get_bom_id().value_or(0);

    // len of serial number plus '/' and max 3-digit number and null
    ArrayStringBuilder<serial_nr.size() + 1 + 3 + 1> sb;
    sb.append_printf("%s/%u", serial_nr.data(), bom_id);
    ChangeInformation(sb.str());
}

MI_OTP_LOVEBOARD::MI_OTP_LOVEBOARD()
    : WindowMenuInfoOtp {
#if HAS_LOVE_BOARD()
    "LoveBoard", data_exchange::get_loveboard_eeprom()
#endif
}
{
}

MI_OTP_XLCD::MI_OTP_XLCD()
    : WindowMenuInfoOtp {
#if HAS_XLCD()
    "xLCD", data_exchange::get_xlcd_eeprom()
#endif
}
{
}

MI_OTP_XBUDDY_EXTENSION::MI_OTP_XBUDDY_EXTENSION()
    : WindowMenuInfoOtp {
#if HAS_XBUDDY_EXTENSION()
    "Extension board", buddy::puppies::xbuddy_extension.get_otp()
#endif
}
{
}

MI_OTP_INDX_HEAD::MI_OTP_INDX_HEAD()
    : WindowMenuInfoOtp {
#if HAS_INDX_HEAD()
    "INDX head", buddy::puppies::indx.get_otp()
#endif
}
{
}

MI_OTP_XL_CAN::MI_OTP_XL_CAN()
    : WindowMenuInfoOtp {
#if HAS_XL_CAN()
    "XL-CAN", buddy::puppies::xl_can.get_otp()
#endif
}
{
#if HAS_XL_CAN()
    set_is_hidden(!buddy::puppies::xl_can.is_enabled());
#endif
}

ScreenMenuBoardInfo::ScreenMenuBoardInfo()
    : ScreenMenuBoardInfo__ {
        _("BOARD INFO"),
        &img::info_16x16,
    } {
    EnableLongHoldScreenAction();
}
