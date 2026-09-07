/// @file
#include "utils.hpp"

#include <feature/openprinttag/tool_tag.hpp>
#include <feature/openprinttag/filament_usage_tracker/filament_usage_tracker.hpp>
#include <bsod/bsod.h>

namespace buddy::openprinttag {

ToolTagStatus tool_tag_status(VirtualToolIndex tool) {
    const auto assigned_tag = ToolTag::for_tool_assigned(tool);
    const auto ephemeral_tag = ToolTag::for_tool_ephemeral(tool);

    if (FilamentType::for_tool(tool) == FilamentType::none) {
        return ToolTagStatus::no_filament;
    }

    if (!assigned_tag.has_value()) {
        return ephemeral_tag.has_value() ? ToolTagStatus::not_assigned_but_present : ToolTagStatus::not_assigned;
    }

    if (assigned_tag != ephemeral_tag) {
        return ephemeral_tag.has_value() ? ToolTagStatus::different_tag_present : ToolTagStatus::tag_missing;
    }

    if (!buddy::openprinttag::filament_usage_tracker().is_tracking(tool)) {
        return ToolTagStatus::tag_problem;
    }

    return ToolTagStatus::ok;
}

#if HAS_GUI()
const img::Resource *tool_tag_status_icon(VirtualToolIndex tool) {
    switch (tool_tag_status(tool)) {

    case ToolTagStatus::ok:
        return &img::openprinttag_white_16x16;

    case ToolTagStatus::no_filament:
    case ToolTagStatus::not_assigned:
        return nullptr;

    case ToolTagStatus::different_tag_present:
    case ToolTagStatus::not_assigned_but_present:
    case ToolTagStatus::tag_problem:
    case ToolTagStatus::tag_missing:
        return &img::openprinttag_orange_16x16;

    case ToolTagStatus::_cnt:
        // Fallback to bsod_unrechable()
        break;
    }

    bsod_unreachable();
}
#endif

} // namespace buddy::openprinttag
