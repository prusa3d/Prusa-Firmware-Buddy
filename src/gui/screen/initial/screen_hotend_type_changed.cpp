/// @file
#include <gui/screen/initial/screen_hotend_type_changed.hpp>

#include <option/has_ht_hotend.h>
static_assert(HAS_HT_HOTEND());

#include <config_store/store_instance.hpp>
#include <hotend_detect.hpp>
#include <hotend_type.hpp>
#include <lang/i18n.h>
#include <sys.hpp>
#include <tool_index.hpp>
#include <window_msgbox.hpp>

static_assert(PhysicalToolIndex::count == 1, "Hotend dialog assumes a single tool");

bool ScreenHotendTypeChanged::should_show() {
    return hotend_detect_dialog_pending;
}

ScreenHotendTypeChanged::ScreenHotendTypeChanged()
    : PseudoScreenCallback {
        [] {
            // hotend_type is read from EEPROM: treat any non-HT value as the standard hotend
            // (the safe, lower-temperature one) — a future-FW value or storage corruption
            // must not soft-brick the printer (config-store enum policy).
            const bool is_high_temp = config_store().hotend_type.get(0) == HotendType::high_temp;

            const char *msg = is_high_temp
                ? N_("Hotend change detected\n\nSelected hotend: High-temp\n\nIs this correct?")
                : N_("Hotend change detected\n\nSelected hotend: Standard\n\nIs this correct?");

            // No = switch to the other hotend and reboot. A standard hotend gets stock_with_sock
            // (its sock is mandatory); HT clears it.
            if (MsgBoxWarning(_(msg), Responses_YesNo) == Response::No) {
                config_store().set_hotend_type_detected(PhysicalToolIndex::from_raw(0),
                    is_high_temp ? HotendType::stock_with_sock : HotendType::high_temp);
                sys_reset(); // Reboot with the new hotend config
            }
            // User confirmed the current hotend — continue booting
        },
    } {}
