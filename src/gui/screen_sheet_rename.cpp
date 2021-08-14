#include "screen_sheet_rename.hpp"
#include "ScreenHandler.hpp"
#include "eeprom.h"
#include "log.h"

static void onclick_ok();
static void onclick_cancel();

uint32_t screen_sheet_rename_t::index_ = 0;
void screen_sheet_rename_t::index(uint32_t index) {
    screen_sheet_rename_t::index_ = index;
}
screen_sheet_rename_t::screen_sheet_rename_t()
    : screen_t()
    , header(this)
    , footer(this)
    , button_ok(this, Rect16(10, 100, 80, 20), onclick_ok, _("OK"))
    , button_cancel(this, Rect16(100, 100, 80, 20), onclick_cancel, _("CANCEL"))
    , text_name(this, Rect16(10, 40, 160, 20), is_multiline::no) {
    char name[MAX_SHEET_NAME_LENGTH];
    memset(name, 0, MAX_SHEET_NAME_LENGTH);
    header.SetText(_("RENAME"));
    sheet_name(screen_sheet_rename_t::index_, name, MAX_SHEET_NAME_LENGTH);
    text_name.SetText(string_view_utf8::MakeCPUFLASH((uint8_t *)name));
    text_name.SetFocus();
    log_info(GUI, "Rename : %s\n", name);
}

void onclick_ok() {
    ///TODO: store count changed characters
    ///uint32_t cnt = sheet_rename(
    log_info(GUI, "Rename Cnt: %d\n", sheet_rename(screen_sheet_rename_t::index_, "anything", 8));
    Screens::Access()->Close();
}

void onclick_cancel() {
    Screens::Access()->Close();
}
