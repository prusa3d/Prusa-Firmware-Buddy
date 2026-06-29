#include "screen_menu_internal_storage.hpp"

#include <i18n.h>
#include <window_msgbox.hpp>
#include <window_dlg_wait.hpp>
#include <buddy/filesystem_littlefs.hpp>
#include <marlin_client.hpp>
#include <transfers/monitor.hpp>

/// The format/test unmount the filesystem under any user of it, and even the
/// read-only check clears the flash driver's sticky error state, which races
/// the littlefs device callbacks of concurrent writers. Refuse while a print
/// or a transfer could be using the storage. A small race window remains
/// between this check and the operation.
static bool storage_busy() {
    return marlin_client::is_printing() || transfers::Monitor::instance.status().has_value();
}

MI_CHECK_INTERNAL_STORAGE::MI_CHECK_INTERNAL_STORAGE()
    : IWindowMenuItem(_("Check Internal Storage"), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_CHECK_INTERNAL_STORAGE::click(IWindowMenu & /*window_menu*/) {
    if (storage_busy()) {
        MsgBoxWarning(_("Not available while a print or a file transfer is in progress."), Responses_Ok);
        return;
    }

    if (internal_storage_check()) {
        MsgBoxInfo(_("Internal storage OK."), Responses_Ok);
    } else {
        MsgBoxError(_("Internal storage check failed. See log."), Responses_Ok);
    }
}

MI_TEST_INTERNAL_STORAGE::MI_TEST_INTERNAL_STORAGE()
    : IWindowMenuItem(_("Test Internal Storage"), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_INTERNAL_STORAGE::click(IWindowMenu & /*window_menu*/) {
    if (!internal_storage_initialized()) {
        MsgBoxError(_("Internal storage is not responding."), Responses_Ok);
        return;
    }

    if (MsgBoxWarning(_("This will erase all data on internal storage. Continue?"), Responses_YesNo, 1) != Response::Yes) {
        return;
    }

    if (storage_busy()) {
        MsgBoxWarning(_("Not available while a print or a file transfer is in progress."), Responses_Ok);
        return;
    }

    InternalStorageTester tester;
    StringViewUtf8Parameters<22> params;
    const auto progress_text = [&] {
        return _("Block %lu / %lu").formatted(params, static_cast<unsigned long>(tester.current_block()), static_cast<unsigned long>(tester.total_blocks()));
    };

    window_dlg_wait_t::wait_until(progress_text(),
        [&](window_dlg_wait_t &dlg) {
            const bool done = tester.step();
            if (tester.phase() == InternalStorageTester::Phase::testing) {
                dlg.set_text(progress_text());
            } else {
                dlg.set_text(_("Restoring filesystem"));
            }
            return done;
        });

    if (!tester.format_succeeded()) {
        MsgBoxError(_("Internal storage test finished, but reformat failed. See log."), Responses_Ok);
    } else if (tester.bad_blocks() == 0) {
        MsgBoxInfo(_("Internal storage test passed."), Responses_Ok);
    } else {
        MsgBoxError(_("Internal storage test failed. See log."), Responses_Ok);
    }
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
