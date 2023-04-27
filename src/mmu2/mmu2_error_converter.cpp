#include "mmu2_error_converter.h"
#include "error_list_mmu.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/buttons.h"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_error_converter.h"
#include "../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"

namespace MMU2 {

static ButtonOperations buttonSelectedOperation = ButtonOperations::NoOperation;

// we don't have a constexpr find_if in C++17/STL yet
template <class InputIt, class UnaryPredicate>
constexpr InputIt find_if_cx(InputIt first, InputIt last, UnaryPredicate p) {
    for (; first != last; ++first) {
        if (p(*first)) {
            return first;
        }
    }
    return last;
}

// Making a constexpr FindError should instruct the compiler to optimize the PrusaErrorCodeIndex
// in such a way that no searching will ever be done at runtime.
// A call to FindError then compiles to a single instruction even on the AVR.
static constexpr uint8_t FindErrorIndex(ErrCode error_code) {
    constexpr uint32_t error_list_size = sizeof(error_list) / sizeof(error_list[0]);
    constexpr auto error_list_end = error_list + error_list_size;
    auto i = find_if_cx(error_list, error_list_end, [error_code](const MMUErrDesc &med) -> bool {
        return med.err_code == error_code;
    });
    return i != error_list_end ? (i - error_list) : (error_list_size - 1);
}

// check that the searching algoritm works
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_DIDNT_TRIGGER) == 0);
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_FILAMENT_STUCK) == 1);
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER) == 2);
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_FILAMENT_STUCK) == 3);

uint8_t PrusaErrorCodeIndex(uint16_t ec) {
    switch (ec) {
    case (uint16_t)ErrorCode::FINDA_DIDNT_SWITCH_ON:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_DIDNT_TRIGGER);
    case (uint16_t)ErrorCode::FINDA_DIDNT_SWITCH_OFF:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_FILAMENT_STUCK);
    case (uint16_t)ErrorCode::FSENSOR_DIDNT_SWITCH_ON:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER);
    case (uint16_t)ErrorCode::FSENSOR_DIDNT_SWITCH_OFF:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_FILAMENT_STUCK);
    case (uint16_t)ErrorCode::FSENSOR_TOO_EARLY:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_TOO_EARLY);
    case (uint16_t)ErrorCode::FINDA_FLICKERS:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_INSPECT_FINDA);
    case (uint16_t)ErrorCode::LOAD_TO_EXTRUDER_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_LOAD_TO_EXTRUDER_FAILED);
    case (uint16_t)ErrorCode::FILAMENT_EJECTED:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FILAMENT_EJECTED);

    case (uint16_t)ErrorCode::STALLED_PULLEY:
    case (uint16_t)ErrorCode::MOVE_PULLEY_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_PULLEY_CANNOT_MOVE);

    case (uint16_t)ErrorCode::HOMING_SELECTOR_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_SELECTOR_CANNOT_HOME);
    case (uint16_t)ErrorCode::MOVE_SELECTOR_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_SELECTOR_CANNOT_MOVE);

    case (uint16_t)ErrorCode::HOMING_IDLER_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_IDLER_CANNOT_HOME);
    case (uint16_t)ErrorCode::MOVE_IDLER_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_IDLER_CANNOT_MOVE);

    case (uint16_t)ErrorCode::MMU_NOT_RESPONDING:
        return FindErrorIndex(ErrCode::ERR_CONNECT_MMU_NOT_RESPONDING);
    case (uint16_t)ErrorCode::PROTOCOL_ERROR:
        return FindErrorIndex(ErrCode::ERR_CONNECT_COMMUNICATION_ERROR);
    case (uint16_t)ErrorCode::FILAMENT_ALREADY_LOADED:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FILAMENT_ALREADY_LOADED);
    case (uint16_t)ErrorCode::INVALID_TOOL:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_INVALID_TOOL);
    case (uint16_t)ErrorCode::QUEUE_FULL:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_QUEUE_FULL);
    case (uint16_t)ErrorCode::VERSION_MISMATCH:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FW_UPDATE_NEEDED);
    case (uint16_t)ErrorCode::INTERNAL:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FW_RUNTIME_ERROR);
    case (uint16_t)ErrorCode::FINDA_VS_EEPROM_DISREPANCY:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_UNLOAD_MANUALLY);
    }

    // Electrical issues which can be detected somehow.
    // Need to be placed before TMC-related errors in order to process couples of error bits between single ones
    // and to keep the code size down.
    if (ec & (uint16_t)ErrorCode::TMC_PULLEY_BIT) {
        if ((ec & (uint16_t)ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) == (uint16_t)ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_MMU_PULLEY_SELFTEST_FAILED);
    } else if (ec & (uint16_t)ErrorCode::TMC_SELECTOR_BIT) {
        if ((ec & (uint16_t)ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) == (uint16_t)ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_MMU_SELECTOR_SELFTEST_FAILED);
    } else if (ec & (uint16_t)ErrorCode::TMC_IDLER_BIT) {
        if ((ec & (uint16_t)ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) == (uint16_t)ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_MMU_IDLER_SELFTEST_FAILED);
    }

    // TMC-related errors - multiple of these can occur at once
    // - in such a case we report the first which gets found/converted into Prusa-Error-Codes (usually the fact, that one TMC has an issue is serious enough)
    // By carefully ordering the checks here we can prioritize the errors being reported to the user.
    if (ec & (uint16_t)ErrorCode::TMC_PULLEY_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_RESET)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_RESET);
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_UNDERVOLTAGE_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_SHORTED);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN)
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_WARNING_TMC_PULLEY_TOO_HOT);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR)
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_TMC_PULLEY_OVERHEAT_ERROR);
    } else if (ec & (uint16_t)ErrorCode::TMC_SELECTOR_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_RESET)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_RESET);
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_UNDERVOLTAGE_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_SHORTED);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN)
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_WARNING_TMC_SELECTOR_TOO_HOT);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR)
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_TMC_SELECTOR_OVERHEAT_ERROR);
    } else if (ec & (uint16_t)ErrorCode::TMC_IDLER_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_RESET)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_RESET);
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_UNDERVOLTAGE_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND)
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_SHORTED);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN)
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_WARNING_TMC_IDLER_TOO_HOT);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR)
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_TMC_IDLER_OVERHEAT_ERROR);
    }

    // if nothing got caught, return a generic error
    return FindErrorIndex(ErrCode::ERR_OTHER_UNKNOWN_ERROR);
}

uint16_t PrusaErrorCode(uint8_t i) {
    return (uint16_t)error_list[i].err_code;
}

const char *PrusaErrorTitle(uint8_t i) {
    return error_list[i].err_title;
}

const char *PrusaErrorDesc(uint8_t i) {
    return error_list[i].err_text;
}

uint8_t PrusaErrorButtons([[maybe_unused]] uint8_t i) {
    return 0; // @@TODO MK4
}

const char *PrusaErrorButtonTitle([[maybe_unused]] uint8_t bi) {
    // -1 represents the hidden NoOperation button which is not drawn in any way
    return nullptr; // @@TODO MK4 (const char *)pgm_read_ptr(btnOperation + bi - 1);
}

const char *PrusaErrorButtonMore() {
    return "More"; // @@TODO MK4
}

const MMUErrDesc &ConvertMMUErrorCode(uint16_t ec) {
    return error_list[PrusaErrorCodeIndex(ec)];
}

Response ConvertMMUButtonOperation(ButtonOperations bo) {
    constexpr uint_fast8_t convItems = 7; // @@TODO probably link with MMU2::ButtonOperations directly
    constexpr std::pair<MMU2::ButtonOperations, Response> conv[convItems] = {
        { MMU2::ButtonOperations::NoOperation, Response::_none },
        { MMU2::ButtonOperations::Retry, Response::Retry },
        { MMU2::ButtonOperations::Continue, Response::Continue },
        { MMU2::ButtonOperations::ResetMMU, Response::Restart },
        { MMU2::ButtonOperations::Unload, Response::Unload },
        { MMU2::ButtonOperations::StopPrint, Response::Stop },
        { MMU2::ButtonOperations::DisableMMU, Response::MMU_disable },
    };
    // @@TODO may be if we force the ButtonOperations to be ordered and directly consecutive, we can do direct indexing
    // But since this function is called 3x to display an MMU Error Screen (i.e. nowhere near a time-critical part)
    // even a sequential search is a viable option here
    auto i = find_if_cx(conv, conv + convItems, [bo](const auto i) { return bo == i.first; });
    return i == conv + convItems ? Response::_none : i->second;
}

struct ResetOnExit {
    ResetOnExit() = default;
    ~ResetOnExit() {
        buttonSelectedOperation = ButtonOperations::NoOperation;
    }
};

Buttons ButtonPressed(uint16_t ec) {
    if (buttonSelectedOperation == ButtonOperations::NoOperation) {
        return NoButton; // no button
    }

    ResetOnExit ros; // clear buttonSelectedOperation on exit from this call
    return ButtonAvailable(ec);
}

Buttons ButtonAvailable(uint16_t ec) {
    uint8_t ei = PrusaErrorCodeIndex(ec);

    // The list of responses which occur in mmu error dialogs
    // Return button index or perform some action on the MK3 by itself (like restart MMU)
    // Based on Prusa-Error-Codes errors_list.h
    // So far hardcoded, but shall be generated in the future
    switch ((ErrCode)PrusaErrorCode(ei)) {
    case ErrCode::ERR_MECHANICAL_FINDA_DIDNT_TRIGGER:
    case ErrCode::ERR_MECHANICAL_FINDA_FILAMENT_STUCK:
        switch (buttonSelectedOperation) {
        case ButtonOperations::Retry: // "Repeat action"
            return Middle;
        case ButtonOperations::Continue: // "Continue"
            return Right;
        default:
            break;
        }
        break;
    case ErrCode::ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER:
    case ErrCode::ERR_MECHANICAL_FSENSOR_FILAMENT_STUCK:
    case ErrCode::ERR_MECHANICAL_FSENSOR_TOO_EARLY:
    case ErrCode::ERR_MECHANICAL_INSPECT_FINDA:
    case ErrCode::ERR_MECHANICAL_SELECTOR_CANNOT_HOME:
    case ErrCode::ERR_MECHANICAL_SELECTOR_CANNOT_MOVE:
    case ErrCode::ERR_MECHANICAL_IDLER_CANNOT_HOME:
    case ErrCode::ERR_MECHANICAL_IDLER_CANNOT_MOVE:
    case ErrCode::ERR_MECHANICAL_PULLEY_CANNOT_MOVE:
    case ErrCode::ERR_SYSTEM_UNLOAD_MANUALLY:
        switch (buttonSelectedOperation) {
        // may be allow move selector right and left in the future
        case ButtonOperations::Retry: // "Repeat action"
            return Middle;
        default:
            break;
        }
        break;
    case ErrCode::ERR_MECHANICAL_LOAD_TO_EXTRUDER_FAILED:
        switch (buttonSelectedOperation) {
        case ButtonOperations::Continue: // User solved the serious mechanical problem by hand - there is no other way around
            return Middle;
        default:
            break;
        }
        break;
    case ErrCode::ERR_TEMPERATURE_WARNING_TMC_PULLEY_TOO_HOT:
    case ErrCode::ERR_TEMPERATURE_WARNING_TMC_SELECTOR_TOO_HOT:
    case ErrCode::ERR_TEMPERATURE_WARNING_TMC_IDLER_TOO_HOT:
        switch (buttonSelectedOperation) {
        case ButtonOperations::Continue: // "Continue"
            return Left;
        case ButtonOperations::ResetMMU: // "Restart MMU"
            return RestartMMU;
        default:
            break;
        }
        break;

    case ErrCode::ERR_TEMPERATURE_TMC_PULLEY_OVERHEAT_ERROR:
    case ErrCode::ERR_TEMPERATURE_TMC_SELECTOR_OVERHEAT_ERROR:
    case ErrCode::ERR_TEMPERATURE_TMC_IDLER_OVERHEAT_ERROR:

    case ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_ERROR:
    case ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_ERROR:
    case ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_ERROR:

    case ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_RESET:
    case ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_RESET:
    case ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_RESET:

    case ErrCode::ERR_ELECTRO_TMC_PULLEY_UNDERVOLTAGE_ERROR:
    case ErrCode::ERR_ELECTRO_TMC_SELECTOR_UNDERVOLTAGE_ERROR:
    case ErrCode::ERR_ELECTRO_TMC_IDLER_UNDERVOLTAGE_ERROR:

    case ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_SHORTED:
    case ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_SHORTED:
    case ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_SHORTED:

    case ErrCode::ERR_ELECTRO_MMU_PULLEY_SELFTEST_FAILED:
    case ErrCode::ERR_ELECTRO_MMU_SELECTOR_SELFTEST_FAILED:
    case ErrCode::ERR_ELECTRO_MMU_IDLER_SELFTEST_FAILED:

    case ErrCode::ERR_CONNECT_MMU_NOT_RESPONDING:
    case ErrCode::ERR_CONNECT_COMMUNICATION_ERROR:

    case ErrCode::ERR_SYSTEM_QUEUE_FULL:
    case ErrCode::ERR_SYSTEM_FW_RUNTIME_ERROR:
        switch (buttonSelectedOperation) {
        case ButtonOperations::ResetMMU: // "Restart MMU"
            return RestartMMU;
        default:
            break;
        }
        break;
    case ErrCode::ERR_SYSTEM_FW_UPDATE_NEEDED:
        switch (buttonSelectedOperation) {
        case ButtonOperations::DisableMMU: // "Disable"
            return DisableMMU;
        default:
            break;
        }
        break;
    case ErrCode::ERR_SYSTEM_FILAMENT_ALREADY_LOADED:
        switch (buttonSelectedOperation) {
        case ButtonOperations::Unload: // "Unload"
            return Left;
        case ButtonOperations::Continue: // "Proceed/Continue"
            return Right;
        default:
            break;
        }
        break;

    case ErrCode::ERR_SYSTEM_INVALID_TOOL:
        switch (buttonSelectedOperation) {
        case ButtonOperations::StopPrint: // "Stop print"
            return StopPrint;
        case ButtonOperations::ResetMMU: // "Restart MMU"
            return RestartMMU;
        default:
            break;
        }
        break;
    case ErrCode::ERR_SYSTEM_FILAMENT_EJECTED:
        switch (buttonSelectedOperation) {
        case ButtonOperations::Continue: // "Continue" - eject filament completed
            return Middle;
        default:
            break;
        }
        break;

    default:
        break;
    }

    return NoButton;
}

void SetButtonResponse(ButtonOperations rsp) {
    buttonSelectedOperation = rsp;
}

} // namespace MMU2
