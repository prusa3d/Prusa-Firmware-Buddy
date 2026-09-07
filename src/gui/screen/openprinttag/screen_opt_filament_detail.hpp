/// @file
#pragma once

#include <client_fsm_types.hpp>
#include <ScreenFactory.hpp>

#include <feature/openprinttag/tool_tag.hpp>

namespace buddy::openprinttag {

/// Returns a Creator for the screen that displays (read-only) print parameters read from the specified tag
ScreenFactory::Creator screen_openprinttag_filament_detail_creator(ToolTag tag);

/// Returns a Creator for the screen that loads the data from a spool to be stored in PendingAdhocFilamentParameters
/// The screen has a confirm button that notifies the Preheat FSM that we've done selecting
ScreenFactory::Creator screen_openprinttag_preheat_mode_creator(ToolTag tag, PreheatMode mode);

}; // namespace buddy::openprinttag
