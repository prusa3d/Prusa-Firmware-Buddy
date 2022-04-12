/**
 * @file window_dlg_load_unload.hpp
 * @author Radek Vana
 * @brief functions to call preheat / load / unload / purge / change in GUI
 * @date 2021-01-24
 */

#pragma once
#include "client_fsm_types.h"
#include "preheat_multithread_status.hpp"

namespace PreheatStatus {

Result DialogBlockingPreheat(RetAndCool_t retAndCool);
Result DialogBlockingLoad(RetAndCool_t retAndCool);
Result DialogBlockingUnLoad(RetAndCool_t retAndCool);
Result DialogBlockingChangeLoad(RetAndCool_t retAndCool);

}
