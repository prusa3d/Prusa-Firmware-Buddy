/// @file
#include "filament_compatibility.hpp"

#include <common/aggregate_arity.hpp>
#include <tool/physical_tool.hpp>
#include <window_msgbox.hpp>

// Needed for ChecksTraits<GeneralCheck>::metadata
using namespace buddy::compatibility_checks;
using namespace buddy::filament_compatibility;

template <>
constinit const ChecksTraits<GeneralCheck>::Metadata ChecksTraits<GeneralCheck>::metadata {
    // FILLME
};

template <>
constinit const ChecksTraits<ToolCheck>::Metadata ChecksTraits<ToolCheck>::metadata {
    {
        ToolCheck::tool_max_temp,
        CheckMetadata {
            .severity = HWCheckSeverity::Abort,
            .title = N_("Tool not high-temperature"),
            .description = N_("Filament requires higher temperatures than what the tool can provide."),
        },
    },
        {
            ToolCheck::abrasive,
            CheckMetadata {
                .severity = HWCheckType::nozzle,
                .title = N_("Filament abrasive"),
                .description = N_("Filament is abrasive, but tool does not have hardened nozzle installed."),
            },
        },
#if HAS_HT_HOTEND()
        {
            ToolCheck::requires_ht_idler_door,
            CheckMetadata {
                .severity = CompatibilityLevel::compatible_with_reminder,
                .title = N_("Check idler door"),
                .description = N_("Filament requires the high-temperature idler door installed on the Nextruder.\n\nMake sure the correct idler door is in place before loading."),
            },
        },
#endif
};

namespace buddy::filament_compatibility {

void CompatibilityReport::generate_noclear(const CompatibilityReportGenerateArgs &args) {
    for (VirtualToolIndex vti : tool_index_iterator(args.tools).skip_all_disabled()) {
        PhysicalTool::for_index(vti.to_physical()).filament_compatibility_report(*this, args);
    }
}

bool CompatibilityReport::visit_failed_checks(const FailedCheckVisitor &visitor) const {
    const auto v = [&](const CheckMetadata &meta) { return visitor(FailedCheck { .meta = &meta }); };

    if (!ChecksTraits<GeneralCheck>::visit_set_bits(failed_general_checks, v)) {
        return false;
    }

    if (!ChecksTraits<ToolCheck>::visit_set_bits(failed_tool_checks, v)) {
        return false;
    }

    return true;
}

void CompatibilityReport::operator|=(const CompatibilityReport &other) {
    static_assert(aggregate_arity<CompatibilityReport>() == 2);
    failed_general_checks |= other.failed_general_checks;
    failed_tool_checks |= other.failed_tool_checks;
}

[[nodiscard]] bool CompatibilityReport::gui_confirm_incompatibility(const FailedCheck &check, Response abort_response) {
    return gui_confirm_incompatibility_default(*check.meta, abort_response);
}

} // namespace buddy::filament_compatibility
