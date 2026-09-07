/// @file
#include "standard_fff_physical_tool.hpp"

#include <tool/hotend/hotend.hpp>
#include <feature/compatibility_checks/filament_compatibility.hpp>

#if BOARD_IS_MASTER_BOARD()
    #include <config_store/store_instance.hpp>
#endif

StandardFFFPhysicalToolBase::StandardFFFPhysicalToolBase(PhysicalToolIndex tool_index, Hotend &hotend)
    : PhysicalTool(ToolType::standard_fff, hotend)
    , tool_index_(tool_index) {}

#if BOARD_IS_MASTER_BOARD()
void StandardFFFPhysicalToolBase::filament_compatibility_report(FilamentCompatibilityReport &report, const FilamentCompatibilityReportGenerateArgs &args) const {
    // Note: accessing the hotend is generally not thread safe, but the filament_compatibility_report is an exception
    // Not using hotend() here to avoid tripping checks if any get implemented
    hotend_ref_.filament_compatibility_report(report, args);

    if (args.filament.is_abrasive && !config_store().get_nozzle_is_hardened(tool_index_)) {
        report.failed_tool_checks.set(buddy::filament_compatibility::ToolCheck::abrasive);
    }

    #if HAS_HT_HOTEND()
    // Bother the user about the HT idler door on filament insertion, as the product requested
    // BFW-8600
    if (args.filament.requires_ht_idler_door && !args.assume_filament_already_inserted) {
        report.failed_tool_checks.set(buddy::filament_compatibility::ToolCheck::requires_ht_idler_door);
    }
    #endif
}
#endif
