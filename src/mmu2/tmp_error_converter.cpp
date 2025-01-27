#include "../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"
#include <string.h>

namespace MMU2 {

void TranslateErr(uint16_t ec, char *dst, size_t dstSize) {
    strlcat(dst, " ", dstSize);

    switch (ec) {
    case (uint16_t)ErrorCode::FINDA_DIDNT_SWITCH_ON: {
        static const char s[] = "FINDAnotSwOn";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::FINDA_DIDNT_SWITCH_OFF: {
        static const char s[] = "FINDAnotSwOff";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::FSENSOR_DIDNT_SWITCH_ON: {
        static const char s[] = "FSnotSwOn";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::FSENSOR_DIDNT_SWITCH_OFF: {
        static const char s[] = "FSnotSwOff";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::FILAMENT_ALREADY_LOADED: {
        static const char s[] = "FilAlrLoaded";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::INVALID_TOOL: {
        static const char s[] = "InvTool";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::QUEUE_FULL: {
        static const char s[] = "QueueFull";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::VERSION_MISMATCH: {
        static const char s[] = "VersMismatch";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::PROTOCOL_ERROR: {
        static const char s[] = "ProtocolErr";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::MMU_NOT_RESPONDING: {
        static const char s[] = "MMUnotResp";
        strlcat(dst, s, dstSize);
    }
        return;
    case (uint16_t)ErrorCode::INTERNAL: {
        static const char s[] = "Internal";
        strlcat(dst, s, dstSize);
    }
        return;
    }

    // TMC-related errors - multiple of these can occur at once
    // - in such a case we report the first which gets found/converted into Prusa-Error-Codes (usually the fact, that one TMC has an issue is serious enough)
    // By carefully ordering the checks here we can prioritize the errors being reported to the user.
    dst[0] = 0;
    if (ec & (uint16_t)ErrorCode::TMC_PULLEY_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH) {
            static const char s[] = "PInit ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_RESET) {
            static const char s[] = "PReset ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP) {
            static const char s[] = "PUndervolt ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND) {
            static const char s[] = "PShort ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN) {
            static const char s[] = "PTempW ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR) {
            static const char s[] = "PTempE ";
            strlcat(dst, s, dstSize);
        }
    }
    if (ec & (uint16_t)ErrorCode::TMC_SELECTOR_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH) {
            static const char s[] = "SInit ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_RESET) {
            static const char s[] = "SReset ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP) {
            static const char s[] = "SUndervolt ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND) {
            static const char s[] = "SShort ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN) {
            static const char s[] = "STempW ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR) {
            static const char s[] = "STempE ";
            strlcat(dst, s, dstSize);
        }
    }
    if (ec & (uint16_t)ErrorCode::TMC_IDLER_BIT) {
        if (ec & (uint16_t)ErrorCode::TMC_IOIN_MISMATCH) {
            static const char s[] = "IInit ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_RESET) {
            static const char s[] = "IReset ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP) {
            static const char s[] = "IUndervolt ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_SHORT_TO_GROUND) {
            static const char s[] = "IShort ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_WARN) {
            static const char s[] = "ITempW ";
            strlcat(dst, s, dstSize);
        }
        if (ec & (uint16_t)ErrorCode::TMC_OVER_TEMPERATURE_ERROR) {
            static const char s[] = "ITempE ";
            strlcat(dst, s, dstSize);
        }
    }
}

} // namespace MMU2
