#include "screen_sheet_rename.hpp"
#include "ScreenHandler.hpp"
#include "eeprom.h"
#include "Marlin/src/core/serial.h"

static void onclick_ok();
static void onclick_cancel();

std::uint32_t screen_sheet_rename_t::index_ = 0;
void screen_sheet_rename_t::index(std::uint32_t index) {
    screen_sheet_rename_t::index_ = index;
}
screen_sheet_rename_t::screen_sheet_rename_t()
    : window_frame_t()
    , header(this)
    , footer(this)
    , button_ok(this, Rect16(10, 100, 80, 20), onclick_ok, _("OK"))
    , button_cancel(this, Rect16(100, 100, 80, 20), onclick_cancel, _("CANCEL"))
    , text_name(this, Rect16(10, 40, 160, 20), is_multiline::no) {
    char name[MAX_SHEET_NAME_LENGTH];
    memset(name, 0, MAX_SHEET_NAME_LENGTH);
    header.SetText(_("RENAME"));
    sheet_name(screen_sheet_rename_t::index_, name, MAX_SHEET_NAME_LENGTH);
    text_name.SetText(string_view_utf8::MakeCPUFLASH((std::uint8_t *)name));
    text_name.SetFocus();
    SERIAL_ECHOPAIR("Rename : ", name);
}

void screen_sheet_rename_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    window_frame_t::windowEvent(sender, event, param);
}

void onclick_ok() {
    std::uint32_t cnt = sheet_rename(
        screen_sheet_rename_t::index_, "muhehe", 6);
    SERIAL_ECHOPAIR("Rename Cnt: ", cnt);
    Screens::Access()->Close();
}

void onclick_cancel() {
    Screens::Access()->Close();
}
