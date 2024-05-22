#pragma once

#include "general_response.hpp"
#include "i18n.h"
#include <printers.h>
#include <common/nozzle_type.hpp>
#include <common/hotend_type.hpp>

inline constexpr const char *get_response_text(Response response) {
    switch (response) {
    case Response::_none:
        return "";
    case Response::Abort:
        return N_("ABORT");
    case Response::Abort_invalidate_test:
        return N_("ABORT");
    case Response::ABS:
        return "ABS";
    case Response::Adjust:
        return N_("Adjust");
    case Response::All:
        return N_("ALL");
    case Response::ASA:
        return "ASA";
    case Response::Back:
        return N_("BACK");
    case Response::Cancel:
        return N_("CANCEL");
    case Response::Change:
        return N_("CHANGE");
    case Response::Continue:
        return N_("CONTINUE");
    case Response::Cooldown:
        return N_("COOLDOWN");
    case Response::Disable:
        return N_("DISABLE");
    case Response::Done:
        return N_("DONE");
    case Response::Filament:
        return N_("FILAMENT");
    case Response::Filament_removed:
        return N_("FILAMENT REMOVED");
    case Response::Finish:
        return N_("FINISH");
    case Response::FLEX:
        return "FLEX";
    case Response::FS_disable:
        return N_("DISABLE FS");
    case Response::NozzleType_Normal:
        return nozzle_type_names[NozzleType::Normal];
    case Response::NozzleType_HighFlow:
        return nozzle_type_names[NozzleType::HighFlow];
    case Response::HIPS:
        return "HIPS";
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
    case Response::NozzleDiameter_04:
        return "0.40 mm";
    case Response::NozzleDiameter_06:
        return "0.60 mm";
    case Response::Ok:
        return N_("OK");
    case Response::Pause:
        return N_("PAUSE");
    case Response::PC:
        return "PC";
    case Response::PETG:
        return "PETG";
    case Response::PLA:
        return "PLA";
    case Response::PP:
        return "PP";
    case Response::Print:
        return "Print";
    case Response::HotendType_Stock:
        return hotend_type_names[HotendType::stock];
    case Response::HotendType_StockWithSock:
        return hotend_type_names[HotendType::stock_with_sock];
    case Response::HotendType_E3DRevo:
        return hotend_type_names[HotendType::e3d_revo];
    case Response::Purge_more:
#if PRINTER_IS_PRUSA_MINI
        return N_("MORE");
#else
        return N_("PURGE MORE");
#endif
    case Response::PVB:
        return "PVB";
    case Response::Quit:
        return N_("QUIT");
    case Response::Reheat:
        return N_("REHEAT");
    case Response::Replace:
        return N_("REPLACE");
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
    case Response::PA:
        return "PA";
    case Response::Postpone5Days:
        return "POSTPONE";
    case Response::PRINT:
        return "PRINT";
    case Response::_count:
        break;
    }
    abort();
}
