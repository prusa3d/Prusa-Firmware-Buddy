/// @file
#pragma once

#include <cstdint>

#include <option/has_gui.h>
#if HAS_GUI()
    #include <img_resources.hpp>
#endif

namespace buddy::openprinttag {

enum class ToolTagStatus : uint8_t {
    /// Everything is fine
    ok,

    /// No filament is loaded to the tool, no tracking is expected
    no_filament,

    /// The loaded filament does not have an OPT tag assigned, so no tracking
    not_assigned,

    /// The loaded filament does not have an OPT tag assigned, but a tag is present
    not_assigned_but_present,

    /// The tag currently at the reader is different to the one originally assigned
    different_tag_present,

    /// The loaded filament has a tag assigned, but it's not present at the reader
    tag_missing,

    /// The tag is
    tag_problem,

    _cnt,
};

/// @returns Status of filament usage tracking for the provided tool
ToolTagStatus tool_tag_status(VirtualToolIndex tool);

#if HAS_GUI()
/// @returns an appropriate OpenPrintTag icon for the provided tool (16x16), based on @p tool_tag_status:
/// - None if the filament is not OPT-assigned/related
/// - Orange if there is some issue with the OPT
/// - White if the OPT is all righty
const img::Resource *tool_tag_status_icon(VirtualToolIndex tool);
#endif

} // namespace buddy::openprinttag
