/// @file
#pragma once

#include <ScreenFactory.hpp>

namespace buddy::openprinttag {

/// Creates an OpenPrintTag info screen for an ephemeral tag - will always fetch information from the present tag, disregarding assignments/loaded filament
ScreenFactory::Creator screen_opt_info_ephemeral_creator(VirtualToolIndex for_tool);

/// Creates a filament info screen with additional information possibly fetched from the OpenPrintTag
ScreenFactory::Creator screen_opt_info_loaded_creator(VirtualToolIndex for_tool);

} // namespace buddy::openprinttag
