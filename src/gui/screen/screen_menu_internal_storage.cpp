#include "screen_menu_internal_storage.hpp"

#include <i18n.h>
#include <window_msgbox.hpp>
#include <buddy/filesystem_littlefs.hpp>
#include <marlin_client.hpp>
#include <transfers/monitor.hpp>

/// The format unmounts the filesystem under any user of it; refuse while
/// a print or a transfer could be using it. A small race window remains
/// between this check and the unmount.
static bool storage_busy() {
    return marlin_client::is_printing() || transfers::Monitor::instance.status().has_value();
}

MI_FORMAT_INTERNAL_STORAGE::MI_FORMAT_INTERNAL_STORAGE()
    : IWindowMenuItem(_("Format Internal Storage"), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FORMAT_INTERNAL_STORAGE::click(IWindowMenu & /*window_menu*/) {
    if (MsgBoxWarning(_("Erase all data on internal storage?"), Responses_YesNo, 1) != Response::Yes) {
        return;
    }

    if (storage_busy()) {
        MsgBoxWarning(_("Not available while a print or a file transfer is in progress."), Responses_Ok);
        return;
    }

    if (!internal_storage_format()) {
        MsgBoxError(_("Internal storage format failed."), Responses_Ok);
        return;
    }

    MsgBoxInfo(_("Internal storage erased."), Responses_Ok);
}

ScreenMenuInternalStorage::ScreenMenuInternalStorage()
    : ScreenMenuInternalStorageBase(_("INTERNAL STORAGE")) {
}
