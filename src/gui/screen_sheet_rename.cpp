#include "screen_sheet_rename.hpp"
#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "log.h"
#include "SteelSheets.hpp"

LOG_COMPONENT_REF(GUI);

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
    char name[MAX_SHEET_NAME_LENGTH + 1];
    memset(name, 0, MAX_SHEET_NAME_LENGTH + 1);
    header.SetText(_("RENAME"));
    SteelSheets::SheetName(screen_sheet_rename_t::index_, name, MAX_SHEET_NAME_LENGTH);
    text_name.SetText(string_view_utf8::MakeCPUFLASH((uint8_t *)name));
    text_name.SetFocus();
    log_info(GUI, "Rename : %s\n", name);
}

void onclick_ok() {
    /// TODO: store count changed characters
    /// uint32_t cnt = sheet_rename(
    unsigned sheet = SteelSheets::RenameSheet(screen_sheet_rename_t::index_, "anything", MAX_SHEET_NAME_LENGTH);
    log_info(GUI, "Rename Cnt: %u\n", sheet);
    Screens::Access()->Close();
}

void onclick_cancel() {
    Screens::Access()->Close();
}
