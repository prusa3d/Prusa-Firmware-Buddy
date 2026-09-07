#pragma once

#include <fsm/print_preview_phases.hpp>
#include <error_codes.hpp>
#include <option/has_wastebin_fill_tracking.h>

constexpr std::optional<ErrCode> map_print_preview_phase_to_error_code(const FSMAndPhase print_preview_phase) {
    if (print_preview_phase.fsm != ClientFSM::PrintPreview) {
        return std::nullopt;
    }

    switch (static_cast<PhasesPrintPreview>(print_preview_phase.phase)) {
    case PhasesPrintPreview::unfinished_selftest:
        return ErrCode::CONNECT_UNFINISHED_SELFTEST;
    case PhasesPrintPreview::filament_not_inserted:
        return ErrCode::CONNECT_PRINT_PREVIEW_NO_FILAMENT;
#if HAS_MMU2()
    case PhasesPrintPreview::mmu_filament_inserted:
        return ErrCode::CONNECT_PRINT_PREVIEW_MMU_FILAMENT_INSERTED;
#endif
#if HAS_E2EE_SUPPORT()
    case PhasesPrintPreview::untrusted_identity:
        return ErrCode::CONNECT_UNTRUSTED_IDENTITY;
#endif
    case PhasesPrintPreview::file_error:
        return ErrCode::CONNECT_PRINT_PREVIEW_FILE_ERROR;
    case PhasesPrintPreview::gcode_incompatible_warning:
    case PhasesPrintPreview::gcode_incompatible_fatal:
        return ErrCode::CONNECT_PRINT_PREVIEW_WRONG_PRINTER;
    case PhasesPrintPreview::filament_incompatible_warning:
    case PhasesPrintPreview::filament_incompatible_fatal:
        return ErrCode::CONNECT_FILAMENT_INCOMPATIBLE;
    case PhasesPrintPreview::wrong_filament:
        return ErrCode::CONNECT_PRINT_PREVIEW_WRONG_FILAMENT;
    case PhasesPrintPreview::new_firmware_available:
        return ErrCode::CONNECT_PRINT_PREVIEW_NEW_FW;
#if HAS_TOOL_MAPPING()
    case PhasesPrintPreview::tools_mapping:
        return ErrCode::CONNECT_PRINT_PREVIEW_TOOLS_MAPPING;
#endif
#if HAS_WASTEBIN_FILL_TRACKING()
    case PhasesPrintPreview::wastebin_overfill_warning:
        // Reported to Connect via its own error code (the on-printer dialog uses the GUI frame text).
        return ErrCode::CONNECT_NOZZLE_CLEANER_MAY_OVERFILL;
    case PhasesPrintPreview::wastebin_emptying:
    case PhasesPrintPreview::wastebin_emptied_returning:
#endif
    case PhasesPrintPreview::loading:
    case PhasesPrintPreview::main_dialog:
    case PhasesPrintPreview::download_wait:
        break;
    }
    return std::nullopt;
}
