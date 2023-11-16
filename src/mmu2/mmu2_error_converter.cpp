#include "mmu2_error_converter.h"
#include "error_list_mmu.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/buttons.h"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_error_converter.h"
#include "../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"
#include "utility_extensions.hpp"

#include <algorithm>

namespace MMU2 {

static ButtonOperations buttonSelectedOperation = ButtonOperations::NoOperation;

// Making a constexpr FindError should instruct the compiler to optimize the PrusaErrorCodeIndex
// in such a way that no searching will ever be done at runtime.
// A call to FindError then compiles to a single instruction even on the AVR.
static consteval uint8_t FindErrorIndex(const ErrCode error_code) {
    const auto i = std::ranges::find_if(error_list, [error_code](const auto &med) { return med.err_code == error_code; });
    if (i != std::end(error_list)) {
        return i - error_list;
    }

    consteval_assert_false("Error not found");
}

// check that the searching algorithm works
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_DIDNT_TRIGGER) == 0);
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_FILAMENT_STUCK) == 1);
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER) == 2);
static_assert(FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_FILAMENT_STUCK) == 3);

constexpr ErrorCode operator&(ErrorCode a, ErrorCode b) {
    return (ErrorCode)((uint16_t)a & (uint16_t)b);
}

constexpr bool ContainsBit(ErrorCode ec, ErrorCode mask) {
    return (uint16_t)ec & (uint16_t)mask;
}

uint8_t PrusaErrorCodeIndex(ErrorCode ec) {
    switch (ec) {
    case ErrorCode::FINDA_DIDNT_SWITCH_ON:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_DIDNT_TRIGGER);
    case ErrorCode::FINDA_DIDNT_SWITCH_OFF:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FINDA_FILAMENT_STUCK);
    case ErrorCode::FSENSOR_DIDNT_SWITCH_ON:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER);
    case ErrorCode::FSENSOR_DIDNT_SWITCH_OFF:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_FILAMENT_STUCK);
    case ErrorCode::FSENSOR_TOO_EARLY:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_FSENSOR_TOO_EARLY);
    case ErrorCode::FINDA_FLICKERS:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_INSPECT_FINDA);
    case ErrorCode::LOAD_TO_EXTRUDER_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_LOAD_TO_EXTRUDER_FAILED);
    case ErrorCode::FILAMENT_EJECTED:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FILAMENT_EJECTED);

    case ErrorCode::STALLED_PULLEY:
    case ErrorCode::MOVE_PULLEY_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_PULLEY_CANNOT_MOVE);

    case ErrorCode::HOMING_SELECTOR_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_SELECTOR_CANNOT_HOME);
    case ErrorCode::MOVE_SELECTOR_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_SELECTOR_CANNOT_MOVE);

    case ErrorCode::HOMING_IDLER_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_IDLER_CANNOT_HOME);
    case ErrorCode::MOVE_IDLER_FAILED:
        return FindErrorIndex(ErrCode::ERR_MECHANICAL_IDLER_CANNOT_MOVE);

    case ErrorCode::MMU_NOT_RESPONDING:
        return FindErrorIndex(ErrCode::ERR_CONNECT_MMU_NOT_RESPONDING);
    case ErrorCode::PROTOCOL_ERROR:
        return FindErrorIndex(ErrCode::ERR_CONNECT_COMMUNICATION_ERROR);
    case ErrorCode::FILAMENT_ALREADY_LOADED:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FILAMENT_ALREADY_LOADED);
    case ErrorCode::INVALID_TOOL:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_INVALID_TOOL);
    case ErrorCode::QUEUE_FULL:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_QUEUE_FULL);
    case ErrorCode::VERSION_MISMATCH:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FW_UPDATE_NEEDED);
    case ErrorCode::INTERNAL:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_FW_RUNTIME_ERROR);
    case ErrorCode::FINDA_VS_EEPROM_DISREPANCY:
        return FindErrorIndex(ErrCode::ERR_SYSTEM_UNLOAD_MANUALLY);
    default:
        break;
    }

    // Electrical issues which can be detected somehow.
    // Need to be placed before TMC-related errors in order to process couples of error bits between single ones
    // and to keep the code size down.
    if (ContainsBit(ec, ErrorCode::TMC_PULLEY_BIT)) {
        if ((ec & ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) == ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_MMU_PULLEY_SELFTEST_FAILED);
        }
    } else if (ContainsBit(ec, ErrorCode::TMC_SELECTOR_BIT)) {
        if ((ec & ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) == ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_MMU_SELECTOR_SELFTEST_FAILED);
        }
    } else if (ContainsBit(ec, ErrorCode::TMC_IDLER_BIT)) {
        if ((ec & ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) == ErrorCode::MMU_SOLDERING_NEEDS_ATTENTION) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_MMU_IDLER_SELFTEST_FAILED);
        }
    }

    // TMC-related errors - multiple of these can occur at once
    // - in such a case we report the first which gets found/converted into Prusa-Error-Codes (usually the fact, that one TMC has an issue is serious enough)
    // By carefully ordering the checks here we can prioritize the errors being reported to the user.
    if (ContainsBit(ec, ErrorCode::TMC_PULLEY_BIT)) {
        if (ContainsBit(ec, ErrorCode::TMC_IOIN_MISMATCH)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_ERROR);
        }
        if (ContainsBit(ec, ErrorCode::TMC_RESET)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_RESET);
        }
        if (ContainsBit(ec, ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_UNDERVOLTAGE_ERROR);
        }
        if (ContainsBit(ec, ErrorCode::TMC_SHORT_TO_GROUND)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_PULLEY_DRIVER_SHORTED);
        }
        if (ContainsBit(ec, ErrorCode::TMC_OVER_TEMPERATURE_WARN)) {
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_WARNING_TMC_PULLEY_TOO_HOT);
        }
        if (ContainsBit(ec, ErrorCode::TMC_OVER_TEMPERATURE_ERROR)) {
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_TMC_PULLEY_OVERHEAT_ERROR);
        }
    } else if (ContainsBit(ec, ErrorCode::TMC_SELECTOR_BIT)) {
        if (ContainsBit(ec, ErrorCode::TMC_IOIN_MISMATCH)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_ERROR);
        }
        if (ContainsBit(ec, ErrorCode::TMC_RESET)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_RESET);
        }
        if (ContainsBit(ec, ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_UNDERVOLTAGE_ERROR);
        }
        if (ContainsBit(ec, ErrorCode::TMC_SHORT_TO_GROUND)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_SELECTOR_DRIVER_SHORTED);
        }
        if (ContainsBit(ec, ErrorCode::TMC_OVER_TEMPERATURE_WARN)) {
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_WARNING_TMC_SELECTOR_TOO_HOT);
        }
        if (ContainsBit(ec, ErrorCode::TMC_OVER_TEMPERATURE_ERROR)) {
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_TMC_SELECTOR_OVERHEAT_ERROR);
        }
    } else if (ContainsBit(ec, ErrorCode::TMC_IDLER_BIT)) {
        if (ContainsBit(ec, ErrorCode::TMC_IOIN_MISMATCH)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_ERROR);
        }
        if (ContainsBit(ec, ErrorCode::TMC_RESET)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_RESET);
        }
        if (ContainsBit(ec, ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_UNDERVOLTAGE_ERROR);
        }
        if (ContainsBit(ec, ErrorCode::TMC_SHORT_TO_GROUND)) {
            return FindErrorIndex(ErrCode::ERR_ELECTRO_TMC_IDLER_DRIVER_SHORTED);
        }
        if (ContainsBit(ec, ErrorCode::TMC_OVER_TEMPERATURE_WARN)) {
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_WARNING_TMC_IDLER_TOO_HOT);
        }
        if (ContainsBit(ec, ErrorCode::TMC_OVER_TEMPERATURE_ERROR)) {
            return FindErrorIndex(ErrCode::ERR_TEMPERATURE_TMC_IDLER_OVERHEAT_ERROR);
        }
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

const MMUErrDesc &ConvertMMUErrorCode(ErrorCode ec) {
    return error_list[PrusaErrorCodeIndex(ec)];
}

static constexpr uint_fast8_t convItems = 7; // @@TODO probably link with MMU2::ButtonOperations directly
static constexpr std::pair<MMU2::ButtonOperations, Response> conv[convItems] = {
    { MMU2::ButtonOperations::NoOperation, Response::_none },
    { MMU2::ButtonOperations::Retry, Response::Retry },
    { MMU2::ButtonOperations::Continue, Response::Continue },
    { MMU2::ButtonOperations::ResetMMU, Response::Restart },
    { MMU2::ButtonOperations::Unload, Response::Unload },
    { MMU2::ButtonOperations::StopPrint, Response::Stop },
    { MMU2::ButtonOperations::DisableMMU, Response::MMU_disable },
};

template <typename RV, typename IN, typename CMP, typename RVEXTRACT>
constexpr RV ResponseConvert(IN /*in*/, RV none, CMP cmp, RVEXTRACT rvextract) {
    const auto i = std::ranges::find_if(conv, cmp);
    return i == (conv + convItems) ? none : rvextract(i);
}

constexpr Response ButtonOperationToResponseCX(ButtonOperations bo) {
    return ResponseConvert(
        bo, Response::_none, [bo](const auto i) { return bo == i.first; }, [](const auto i) { return i->second; });
}

constexpr ButtonOperations ResponseToButtonOperationsCX(Response rsp) {
    return ResponseConvert(
        rsp, ButtonOperations::NoOperation, [rsp](const auto i) { return rsp == i.second; }, [](const auto i) { return i->first; });
}

// make sure the bidirectional translation works to full extent
static_assert(ButtonOperationToResponseCX(MMU2::ButtonOperations::NoOperation) == Response::_none);
static_assert(ButtonOperationToResponseCX(MMU2::ButtonOperations::Retry) == Response::Retry);
static_assert(ButtonOperationToResponseCX(MMU2::ButtonOperations::Continue) == Response::Continue);
static_assert(ButtonOperationToResponseCX(MMU2::ButtonOperations::ResetMMU) == Response::Restart);
static_assert(ButtonOperationToResponseCX(MMU2::ButtonOperations::Unload) == Response::Unload);
static_assert(ButtonOperationToResponseCX(MMU2::ButtonOperations::StopPrint) == Response::Stop);
static_assert(ButtonOperationToResponseCX(MMU2::ButtonOperations::DisableMMU) == Response::MMU_disable);

static_assert(ResponseToButtonOperationsCX(Response::_none) == MMU2::ButtonOperations::NoOperation);
static_assert(ResponseToButtonOperationsCX(Response::Retry) == MMU2::ButtonOperations::Retry);
static_assert(ResponseToButtonOperationsCX(Response::Continue) == MMU2::ButtonOperations::Continue);
static_assert(ResponseToButtonOperationsCX(Response::Restart) == MMU2::ButtonOperations::ResetMMU);
static_assert(ResponseToButtonOperationsCX(Response::Unload) == MMU2::ButtonOperations::Unload);
static_assert(ResponseToButtonOperationsCX(Response::Stop) == MMU2::ButtonOperations::StopPrint);
static_assert(ResponseToButtonOperationsCX(Response::MMU_disable) == MMU2::ButtonOperations::DisableMMU);

Response ButtonOperationToResponse(ButtonOperations bo) {
    return ButtonOperationToResponseCX(bo);
}

ButtonOperations ResponseToButtonOperations(Response rsp) {
    return ResponseToButtonOperationsCX(rsp);
}

Buttons ButtonPressed(ErrorCode ec) {
    if (buttonSelectedOperation == ButtonOperations::NoOperation) {
        return Buttons::NoButton; // no button
    }

    const auto result = ButtonAvailable(ec);
    buttonSelectedOperation = ButtonOperations::NoOperation; // Reset operation
    return result;
}

Buttons ButtonAvailable(ErrorCode ec) {
    const MMUErrDesc &d = ConvertMMUErrorCode(ec);
    auto bi = std::find_if(d.buttons.begin(), d.buttons.end(), [&](ButtonOperations b) { return b == buttonSelectedOperation; });
    if (bi == d.buttons.end()) {
        return Buttons::NoButton;
    }
    // there is some button - it is either a Left/Middle/Right (consumed by the MMU) or a special one (consumed by the printer)
    // so some hand tweaking is necessary
    switch (*bi) {
    case ButtonOperations::ResetMMU:
        return Buttons::ResetMMU;
    case ButtonOperations::StopPrint:
        return Buttons::StopPrint;
    case ButtonOperations::DisableMMU:
        return Buttons::DisableMMU;
    default: // by default return the index of the button - which corresponds to Left/Middle/Right
        static_assert((uint8_t)Buttons::Right == 0);
        static_assert((uint8_t)Buttons::Middle == 1);
        static_assert((uint8_t)Buttons::Left == 2);
        return (Buttons)std::distance(d.buttons.begin(), bi);
    }
}

void SetButtonResponse(ButtonOperations rsp) {
    buttonSelectedOperation = rsp;
}

} // namespace MMU2
