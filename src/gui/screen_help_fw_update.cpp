/**
 * @file screen_help_fw_update.cpp
 */

#include "screen_help_fw_update.hpp"
#include "ScreenHandler.hpp"
#include <config_store/store_instance.hpp>

inline constexpr PhaseResponses Responses_Back = { Response::Back, Response::_none, Response::_none, Response::_none };

constexpr size_t row_0 = 44;

#if defined(USE_ILI9488)
constexpr size_t descr_h = resource_font_size(IDR_FNT_NORMAL).h * 8;
constexpr size_t row_1 = row_0 + descr_h + resource_font_size(IDR_FNT_NORMAL).h / 2;

constexpr size_t qr_size = 130;
constexpr size_t col_0 = 30;
constexpr size_t col_0_w = 285;
constexpr size_t col_1 = 320;
constexpr size_t col_1_w = qr_size;
constexpr size_t col_0_1_gap = col_1 - col_0 - col_0_w;
constexpr size_t tot_w = col_0_w + col_1_w + col_0_1_gap;

static const constexpr Rect16 descr_rect = Rect16(col_0, row_0, col_0_w, descr_h);
static const constexpr Rect16 QR_rect = Rect16(col_1, row_0, qr_size, qr_size);
static const constexpr Rect16 help_rect = Rect16(col_0, row_1, tot_w, resource_font_size(IDR_FNT_NORMAL).h * 3);
static constexpr const char *txt_descr = N_("Download and copy the firmware (.bbf) file to the USB flash drive. Insert the drive into the printer and turn it on or restart it. Confirm the installation of the new firmware.");
#elif defined(USE_ST7789)
constexpr size_t qr_size = 100;
constexpr size_t col_0 = 10;
constexpr size_t col_0_w = 120;
constexpr size_t col_1 = 130;
constexpr size_t col_1_w = qr_size;
constexpr size_t tot_w = col_0_w + col_1_w;

constexpr size_t descr1_h = resource_font_size(IDR_FNT_SPECIAL).h * 5;
constexpr size_t descr2_h = resource_font_size(IDR_FNT_SPECIAL).h * 4;
constexpr size_t row_1 = row_0 + descr1_h + descr2_h + resource_font_size(IDR_FNT_SPECIAL).h + 3 /*Visual alignment*/;
static const constexpr Rect16 descr_rect = Rect16(col_0, row_0, col_0_w, descr1_h);
static const constexpr Rect16 descr_rect2 = Rect16(col_0, row_0 + descr1_h + 13 /*Visual alignment*/, col_0_w + col_1_w + 5 /*Visual alignment*/, descr2_h);
static const constexpr Rect16 QR_rect = Rect16(col_1, row_0, qr_size, qr_size);
static const constexpr Rect16 help_rect = Rect16(col_0, row_1, tot_w, resource_font_size(IDR_FNT_SPECIAL).h * 4);
static constexpr const char *txt_descr = N_("Download the firmware (.bbf) file to the USB flash drive.");
static constexpr const char *txt_descr2 = N_("Insert the drive into the printer and turn it on or restart it. Confirm the installation.");
#endif

#if PRINTER_IS_PRUSA_MK4
static const char *get_txt_qr() {
    return config_store().xy_motors_400_step.get() ? "prusa.io/mk4-firmware" : "prusa.io/mk3.9-firmware";
}
static const char *get_txt_help() {
    return config_store().xy_motors_400_step.get() ? N_("To learn more including firmware downgrade, please visit:\nprusa.io/mk4-firmware")
                                                   : N_("To learn more including firmware downgrade, please visit:\nprusa.io/mk3.9-firmware");
}
#endif
#if PRINTER_IS_PRUSA_MK3_5
static constexpr const char *get_txt_qr() {
    return "prusa.io/mk3.5-firmware";
}
static constexpr const char *get_txt_help() {
    return N_("To learn more including firmware downgrade, please visit:\nprusa.io/mk3.5-firmware");
}
#endif
#if PRINTER_IS_PRUSA_XL
static constexpr const char *get_txt_qr() {
    return "prusa.io/xl-firmware";
}
static constexpr const char *get_txt_help() {
    return N_("To learn more including firmware downgrade, please visit:\nprusa.io/xl-firmware");
}
#endif
#if PRINTER_IS_PRUSA_MINI
static constexpr const char *get_txt_qr() {
    return "prusa.io/mini-firmware";
}
static constexpr const char *get_txt_help() {
    return N_("To learn more including firmware downgrade, please visit: prusa.io/mini-firmware");
}
#endif
static constexpr const char *txt_header = N_("How to update firmware?");

ScreenHelpFWUpdate::ScreenHelpFWUpdate()
    : AddSuperWindow<screen_t>()
    , header(this)
    , description(this, descr_rect, is_multiline::yes)
#if defined(USE_ST7789)
    , description2(this, descr_rect2, is_multiline::yes)
#endif
    , help(this, help_rect, is_multiline::yes)
    , qr(this, QR_rect, 1, Align_t::RightTop())
    , radio(this, GuiDefaults::GetButtonRect(GetRect()), Responses_Back) {
    CaptureNormalWindow(radio);

#if defined(USE_ST7789)
    description.set_font(resource_font(IDR_FNT_SPECIAL));
    help.set_font(resource_font(IDR_FNT_SPECIAL));
    description2.set_font(resource_font(IDR_FNT_SPECIAL));
    description2.SetText(_(txt_descr2));
#endif

    header.SetIcon(&img::info_16x16);
    header.SetText(_(txt_header));

    qr.SetText(get_txt_qr());

    description.SetAlignment(Align_t::LeftTop());
    description.SetText(_(txt_descr));

    help.SetAlignment(Align_t::LeftTop());
    help.SetText(_(get_txt_help()));
}

void ScreenHelpFWUpdate::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CLICK:
        Screens::Access()->Close();
        break;
    default:
        break;
    }
}
