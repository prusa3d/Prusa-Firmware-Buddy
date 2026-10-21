#include "tool_tag.hpp"

#include <config_store/store_instance.hpp>
#include <feature/openprinttag/request_manager.hpp>

#include <option/has_anfc_for_multiple_tools.h>
#include <option/has_mmu2.h>

namespace buddy::openprinttag {

bool has_tool_openprinttag_reader([[maybe_unused]] VirtualToolIndex tool) {

#if HAS_MMU2() && !HAS_ANFC_FOR_MULTIPLE_TOOLS()
    return !config_store().mmu2_enabled.get();
#else
    return true;
#endif
}

std::optional<ToolTag> ToolTag::for_tool_ephemeral(VirtualToolIndex tool) {
    if (!has_tool_openprinttag_reader(tool)) {
        return std::nullopt;
    }

    const auto tag_uid = manager().get_tag_uid_for_tool(tool);
    if (tag_uid) {
        const auto uid_hash = tag_uid->hash();
        return ToolTag { tool, uid_hash };
    } else {
        return std::nullopt;
    }
}

std::optional<ToolTag> ToolTag::for_tool_assigned(VirtualToolIndex tool) {
    // Note: Using FilamentTypeParameters::openprinttag_uid_hash would be more correct,
    // but that would require computing the whole parameters. This is functionally equivalent and way more sleek

    if (!has_tool_openprinttag_reader(tool)) {
        return std::nullopt;
    }

    if (FilamentType::for_tool(tool) != AdHocFilamentType { tool }) {
        // adhoc_filament_assigned_openprinttag is available only if the appropriate ad-hoc filament type is used
        return std::nullopt;
    }

    // Validate the config store item match
    using HashItem = decltype(config_store().adhoc_filament_assigned_openprinttag);
    static_assert(std::is_same_v<HashItem::value_type, UIDHash>);
    static_assert(HashItem::default_val == no_tag_hash);

    const auto uid_hash = config_store().adhoc_filament_assigned_openprinttag.get(tool.to_raw());
    if (uid_hash == no_tag_hash) {
        return std::nullopt;
    }

    return ToolTag { tool, uid_hash };
}

} // namespace buddy::openprinttag
