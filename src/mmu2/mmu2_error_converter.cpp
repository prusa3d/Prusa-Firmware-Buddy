#include "mmu2_error_converter.h"
#include "../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"

namespace MMU2 {

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

// Making a constexpr FindError should instruct the compiler to optimize the ConvertMMUErrorCode
// in such a way that no searching will ever be done at runtime.
// A call to FindError then compiles to a single LDR instruction.
static constexpr const MMUErrorDesc &FindError(uint32_t pec) {
    constexpr uint32_t error_list_size = sizeof(error_list) / sizeof(error_list[0]);
    constexpr auto error_list_end = error_list + error_list_size;
    auto i = find_if_cx(error_list, error_list_end, [pec](const MMUErrorDesc &med) -> bool {
        return med.err_num == pec;
    });
    return i != error_list_end ? *i : error_list[error_list_size - 1];
}

// hand-crafted recoding of MMU error codes into unified Prusa-Error-Codes
// Subject to transform into a constexpr map once the code is verified
const MMUErrorDesc &ConvertMMUErrorCode(uint16_t ec) {

    switch (ec) {
    case (uint16_t)ErrorCode::FINDA_DIDNT_SWITCH_ON:
        return FindError(ERR_MECHANICAL_FINDA_DIDNT_TRIGGER);
    case (uint16_t)ErrorCode::FINDA_DIDNT_SWITCH_OFF:
        return FindError(ERR_MECHANICAL_FINDA_DIDNT_SWITCH_OFF);
    case (uint16_t)ErrorCode::FSENSOR_DIDNT_SWITCH_ON:
        return FindError(ERR_MECHANICAL_FSENSOR_DIDNT_TRIGGER);
    case (uint16_t)ErrorCode::FSENSOR_DIDNT_SWITCH_OFF:
        return FindError(ERR_MECHANICAL_FSENSOR_DIDNT_SWITCH_OFF);
    case (uint16_t)ErrorCode::STALLED_PULLEY:
        return FindError(ERR_MECHANICAL_PULLEY_STALLED);
    case (uint16_t)ErrorCode::HOMING_SELECTOR_FAILED:
        return FindError(ERR_MECHANICAL_SELECTOR_CANNOT_HOME);
    case (uint16_t)ErrorCode::HOMING_IDLER_FAILED:
        return FindError(ERR_MECHANICAL_IDLER_CANNOT_HOME);
    case (uint16_t)ErrorCode::FILAMENT_ALREADY_LOADED:
        return FindError(ERR_SYSTEM_FILAMENT_ALREADY_LOADED);
    case (uint16_t)ErrorCode::INVALID_TOOL:
        return FindError(ERR_SYSTEM_INVALID_TOOL);
    case (uint16_t)ErrorCode::QUEUE_FULL:
        return FindError(ERR_SYSTEM_QUEUE_FULL);
    case (uint16_t)ErrorCode::VERSION_MISMATCH:
        return FindError(ERR_SYSTEM_VERSION_MISMATCH);
    case (uint16_t)ErrorCode::PROTOCOL_ERROR:
        return FindError(ERR_CONNECT_COMMUNICATION_ERROR);
    case (uint16_t)ErrorCode::MMU_NOT_RESPONDING:
        return FindError(ERR_CONNECT_MMU_NOT_RESPONDING);
    case (uint16_t)ErrorCode::INTERNAL:
        return FindError(ERR_SYSTEM_RUNTIME_ERROR);
    }

    // TMC-related errors - multiple of these can occur at once
    // - in such a case we report the first which gets found/converted into Prusa-Error-Codes (usually the fact, that one TMC has an issue is serious enough)
    // By carefully ordering the checks here we can prioritize the errors being reported to the user.
    if (ec & (uint16_t)ErrorCode::TMC_PULLEY_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH)
            return FindError(ERR_ELECTRICAL_TMC_PULLEY_DRIVER_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_RESET)
            return FindError(ERR_ELECTRICAL_TMC_PULLEY_DRIVER_RESET);
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)
            return FindError(ERR_ELECTRICAL_TMC_PULLEY_UNDERVOLTAGE_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND)
            return FindError(ERR_ELECTRICAL_TMC_PULLEY_DRIVER_SHORTED);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN)
            return FindError(ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_WARN);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR)
            return FindError(ERR_TEMPERATURE_TMC_PULLEY_OVER_TEMPERATURE_ERROR);
    } else if (ec & (uint16_t)ErrorCode::TMC_SELECTOR_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH)
            return FindError(ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_RESET)
            return FindError(ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_RESET);
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)
            return FindError(ERR_ELECTRICAL_TMC_SELECTOR_UNDERVOLTAGE_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND)
            return FindError(ERR_ELECTRICAL_TMC_SELECTOR_DRIVER_SHORTED);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN)
            return FindError(ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_WARN);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR)
            return FindError(ERR_TEMPERATURE_TMC_SELECTOR_OVER_TEMPERATURE_ERROR);
    } else if (ec & (uint16_t)ErrorCode::TMC_IDLER_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH)
            return FindError(ERR_ELECTRICAL_TMC_IDLER_DRIVER_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_RESET)
            return FindError(ERR_ELECTRICAL_TMC_IDLER_DRIVER_RESET);
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP)
            return FindError(ERR_ELECTRICAL_TMC_IDLER_UNDERVOLTAGE_ERROR);
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND)
            return FindError(ERR_ELECTRICAL_TMC_IDLER_DRIVER_SHORTED);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN)
            return FindError(ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_WARN);
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR)
            return FindError(ERR_TEMPERATURE_TMC_IDLER_OVER_TEMPERATURE_ERROR);
    }

    // if nothing got caught, return a generic error
    return FindError(ERR_OTHER);
}

Response ConvertMMUButtonOperation(ButtonOperations bo) {
    constexpr uint_fast8_t convItems = 8; // @@TODO probably link with MMU2::ButtonOperations directly
    constexpr std::pair<MMU2::ButtonOperations, Response> conv[convItems] = {
        { MMU2::ButtonOperations::NoOperation, Response::_none },
        { MMU2::ButtonOperations::Retry, Response::Retry },
        { MMU2::ButtonOperations::SlowLoad, Response::Slowly },
        { MMU2::ButtonOperations::Continue, Response::Continue },
        { MMU2::ButtonOperations::RestartMMU, Response::Restart },
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

} // namespace MMU2
