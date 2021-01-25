/*
 * window_dlg_preheat.hpp
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#pragma once

#include "filament.hpp"
#include "../../lang/string_view_utf8.hpp"

filament_t gui_dlg_preheat(string_view_utf8 caption);
filament_t gui_dlg_preheat_autoselect_if_able(string_view_utf8 caption);
filament_t gui_dlg_preheat_forced(string_view_utf8 caption);                    //no return option
filament_t gui_dlg_preheat_autoselect_if_able_forced(string_view_utf8 caption); //no return option
