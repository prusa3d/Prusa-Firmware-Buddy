#pragma once

#include "general_response.hpp"
#include "i18n.h"
#include <printers.h>

inline constexpr const char *get_response_text(Response response) {
    switch (response) {
    case Response::_none:
        return "";
    case Response::Abort:
        return N_("ABORT");
    case Response::Abort_invalidate_test:
        return N_("ABORT");
    case Response::Adjust:
        return N_("ADJUST");
    case Response::All:
        return N_("ALL");
    case Response::Always:
        return N_("ALWAYS");
    case Response::Back:
        return N_("BACK");
    case Response::Calibrate:
        return N_("CALIBRATE");
    case Response::Cancel:
        return N_("CANCEL");
    case Response::Help:
        return N_("HELP");
    case Response::Change:
        return N_("CHANGE");
    case Response::Continue:
        return N_("CONTINUE");
    case Response::Cooldown:
        return N_("COOLDOWN");
    case Response::Details:
        return N_("DETAILS");
    case Response::Disable:
        return N_("DISABLE");
    case Response::Done:
        return N_("DONE");
    case Response::Empty:
        return N_("EMPTY");
    case Response::Filament:
        return N_("FILAMENT");
    case Response::Filament_removed:
        return N_("FILAMENT REMOVED");
    case Response::Finish:
        return N_("FINISH");
    case Response::FS_disable:
        return N_("DISABLE FS");
    case Response::Ignore:
        return N_("IGNORE");
    case Response::Left:
        return N_("LEFT");
    case Response::Load:
        return N_("LOAD");
    case Response::MMU_disable:
        return N_("DISABLE MMU");
    case Response::Never:
        return N_("NEVER");
    case Response::Next:
        return N_("NEXT");
    case Response::No:
        return N_("NO");
    case Response::NotNow:
        return N_("NOT NOW");
    case Response::Ok:
        return N_("OK");
    case Response::Pause:
        return N_("PAUSE");
    case Response::Print:
        return N_("PRINT");
    case Response::Purge_more:
#if PRINTER_IS_PRUSA_MINI()
        return N_("MORE");
#else
        return N_("PURGE MORE");
#endif
    case Response::Quit:
        return N_("QUIT");
    case Response::Reheat:
        return N_("REHEAT");
    case Response::Replace:
        return N_("REPLACE");
    case Response::Remap:
        return N_("REMAP");
    case Response::Remove:
        return N_("REMOVE");
    case Response::Restart:
        return N_("RESTART");
    case Response::Resume:
        return N_("RESUME");
    case Response::Retry:
        return N_("RETRY");
    case Response::Right:
        return N_("RIGHT");
    case Response::Skip:
        return N_("SKIP");
    case Response::Slowly:
        return N_("SLOWLY");
    case Response::SpoolJoin:
        return N_("SPOOL JOIN");
    case Response::Stop:
        return N_("STOP");
    case Response::Unload:
        return N_("UNLOAD");
    case Response::Yes:
        return N_("YES");
    case Response::Heatup:
        return N_("HEATUP");
    case Response::Postpone5Days:
        return N_("POSTPONE");
    case Response::Docks4:
        return N_("4 DOCKS");
    case Response::Docks8:
        return N_("8 DOCKS");
    case Response::Other:
        return N_("OTHER");

    case Response::_count:
        break;
    }
    abort();
}
