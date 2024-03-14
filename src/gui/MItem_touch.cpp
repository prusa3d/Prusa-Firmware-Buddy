/**
 * @file MItem_touch.cpp
 */
#include "MItem_touch.hpp"
#include "touch_get.hpp"
#include "touch_dependency.hpp"
#include "window_msgbox.hpp"
#include "img_resources.hpp"
#include "ScreenHandler.hpp"
#include <device/peripherals.h>

static inline void MsgBoxNonBlockInfo(string_view_utf8 txt) {
    constexpr static const char *title = N_("Information");
    MsgBoxTitled mbt(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, txt, is_multiline::yes, _(title), &img::info_16x16);
    gui::TickLoop();
    gui_loop();
}

/*****************************************************************************/
// MI_SAVE_TOUCH
MI_SAVE_TOUCH::MI_SAVE_TOUCH()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_SAVE_TOUCH::click(IWindowMenu & /*window_menu*/) {
    MsgBoxNonBlockInfo(_("Touch registers are being saved."));
    if (touch::download_as_new()) {
        MsgBoxInfo(_("Touch registers (file touch.bin) were saved to the USB drive."), Responses_Ok);
    } else {
        MsgBoxError(_("Error saving touch registers to the USB drive. Please reinsert the USB drive and try again."), Responses_Ok);
    }
}

/*****************************************************************************/
// MI_LOAD_TOUCH
MI_LOAD_TOUCH::MI_LOAD_TOUCH()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_LOAD_TOUCH::click(IWindowMenu & /*window_menu*/) {
    MsgBoxNonBlockInfo(_("Touch registers are being loaded."));
    if (touch::upload("/usb/touch.bin")) {
        MsgBoxInfo(_("Touch registers (file touch.bin) were uploaded from the USB drive."), Responses_Ok);
    } else {
        MsgBoxError(_("Error uploading touch registers from the USB drive. Please reinsert the USB drive and try again."), Responses_Ok);
    }
}

/*****************************************************************************/
// MI_RESET_TOUCH
MI_RESET_TOUCH::MI_RESET_TOUCH()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_RESET_TOUCH::click(IWindowMenu & /*window_menu*/) {
    MsgBoxNonBlockInfo(_("Touch registers are being loaded."));
    touch::set_registers();
    MsgBoxInfo(_("Touch registers were reset."), Responses_Ok);
}

/*****************************************************************************/
// MI_DISP_RST
MI_DISP_RST::MI_DISP_RST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_DISP_RST::click([[maybe_unused]] IWindowMenu &window_menu) {
    display::Init();
    Screens::Access()->SetDisplayReinitialized();
}

/*****************************************************************************/
// MI_ENABLE_TOUCH
MI_ENABLE_TOUCH::MI_ENABLE_TOUCH()
    : WI_ICON_SWITCH_OFF_ON_t(touch::is_enabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_ENABLE_TOUCH::OnChange([[maybe_unused]] size_t old_index) {
    if (index != 0) {
        // if I call busy flag clear workaround at this moment it brakes
        touch::enable();
    } else {
        touch::disable();
    }
}

static const SpinConfigInt touch_err_cnf = SpinConfigInt({ { 0, INT_MAX, 1 } }, "", spin_off_opt_t::no);

/*****************************************************************************/
// MI_TOUCH_ERR_COUNT
MI_TOUCH_ERR_COUNT::MI_TOUCH_ERR_COUNT()
    : WiSpinInt(0, touch_err_cnf, string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::no, is_hidden_t::dev) {
}

/*****************************************************************************/
// MI_I2C_WORKAROUND
MI_I2C_WORKAROUND::MI_I2C_WORKAROUND()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_I2C_WORKAROUND::click([[maybe_unused]] IWindowMenu &window_menu) {
    __HAL_RCC_I2C3_RELEASE_RESET();
}

/*****************************************************************************/
// MI_I2C_FORCE_RESET
MI_I2C_FORCE_RESET::MI_I2C_FORCE_RESET()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_I2C_FORCE_RESET::click([[maybe_unused]] IWindowMenu &window_menu) {
    __HAL_RCC_I2C3_FORCE_RESET();
}

/*****************************************************************************/
// MI_I2C_RELEASE_FORCE_RESET
MI_I2C_RELEASE_FORCE_RESET::MI_I2C_RELEASE_FORCE_RESET()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_I2C_RELEASE_FORCE_RESET::click([[maybe_unused]] IWindowMenu &window_menu) {
    __HAL_RCC_I2C3_RELEASE_RESET();
}

/*****************************************************************************/
// MI_DISPI2C_RST
MI_DISPI2C_RST::MI_DISPI2C_RST()
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_DISPI2C_RST::click([[maybe_unused]] IWindowMenu &window_menu) {
    I2C_INIT(touch);
}
