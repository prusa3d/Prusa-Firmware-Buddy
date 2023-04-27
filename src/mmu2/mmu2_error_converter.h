#pragma once
#include <stdint.h>
#include "error_codes_mmu.hpp"
#include "../src/common/general_response.hpp"

namespace MMU2 {

/// Converts MMU error codes into Prusa-Error-Codes
/// These numbered error lists are separate and almost unrelated repositories and principles
/// thus they shouldn't be explicitly connected in one or the other repository.
/// E.g. the MMU can send multiple TMC errors at once, which is not possible to model in Prusa-Error-Codes.
/// Such a situation will be resolved by printing one of the errors whichever gets detected first from the MMU's error code.
const MMUErrDesc &ConvertMMUErrorCode(uint16_t ec);

/// Converts MMU button operations codes (defined in Prusa-Error-Codes) into a Response
Response ConvertMMUButtonOperation(ButtonOperations bo);

} // namespace MMU2
