/// @file
/// @brief additional wrappers converting MMU error codes and their corresponding buttons into GUI types and structures
///
/// Cannot be unified with the 8-bit FW, therefore it is a separate header in Buddy FW.
/// Uses the mmu2_error_converter.h which is common to 8-bit FW and Buddy FW

#pragma once
#include <stdint.h>
#include "error_codes_mmu.hpp"
#include "../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"
#include "../src/common/general_response.hpp"

namespace MMU2 {

/// Converts MMU error codes into Prusa-Error-Codes
/// These numbered error lists are separate and almost unrelated repositories and principles
/// thus they shouldn't be explicitly connected in one or the other repository.
/// E.g. the MMU can send multiple TMC errors at once, which is not possible to model in Prusa-Error-Codes.
/// Such a situation will be resolved by printing one of the errors whichever gets detected first from the MMU's error code.
const MMUErrDesc &ConvertMMUErrorCode(ErrorCode ec);

/// Converts MMU button operations codes (defined in Prusa-Error-Codes) into a Response
Response ButtonOperationToResponse(ButtonOperations bo);

/// Reverse conversion
ButtonOperations ResponseToButtonOperations(Response rsp);

} // namespace MMU2
