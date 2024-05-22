#include "catch2/catch.hpp"

#include <protocol_logic.h>
#include "stub_interfaces.h"

// extracted from the unit tests of the protocol itself
std::string AppendCRC(const std::string_view s, uint8_t crc) {
    char tmp[5]; // *ffn are 4 bytes + \0 at the end
    snprintf(tmp, 5, "*%x\n", (unsigned)crc);
    return std::string(s) + std::string(tmp);
}

std::string AppendCRCWrite(const std::string_view src) {
    uint8_t crc = modules::crc::CRC8::CCITT_updateCX(0, src[0]);
    // scan hex value
    size_t charsRead = 0;
    uint8_t rqValue = std::stoul(src.data() + 1, &charsRead, 16);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, rqValue);
    if (src[2 + charsRead] == 'A') {
        // response confirmation
        crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
        crc = modules::crc::CRC8::CCITT_updateCX(crc, src[2 + charsRead]); // param code
        crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
        return AppendCRC(src, crc);
    } else {
        uint16_t paramValue = std::stoul(src.data() + 2 + charsRead, nullptr, 16);
        crc = modules::crc::CRC8::CCITT_updateW(crc, paramValue);
        return AppendCRC(src, crc);
    }
}

std::string AppendCRC(const std::string_view src) {
    // this code basically needs parsing of the input text and compute the CRC from the parsed data
    REQUIRE(src.size() > 1);
    // code
    if (src[0] == 'W') {
        return AppendCRCWrite(src);
    }

    size_t charsRead = 0;
    uint8_t crc = modules::crc::CRC8::CCITT_updateCX(0, src[0]);
    // scan hex value
    uint8_t rqValue = std::stoul(src.data() + 1, &charsRead, 16);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, rqValue);
    crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
    if (!src[1 + charsRead]) {
        return AppendCRC(src, crc); // eof
    }
    // [2] is a space
    REQUIRE(src.size() > 2 + charsRead);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, src[2 + charsRead]); // param code
    if (!src[3 + charsRead]) {
        crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
        return AppendCRC(src, crc); // eof
    }
    REQUIRE(src.size() > 3 + charsRead);
    uint16_t paramValue = std::stoul(src.data() + 3 + charsRead, nullptr, 16);
    crc = modules::crc::CRC8::CCITT_updateW(crc, paramValue);
    return AppendCRC(src, crc);
}

// std::string AppendCRCWrite(const std::string_view src) {
//     // this code basically needs parsing of the input text and compute the CRC from the parsed data
//     REQUIRE(src.size() > 3);
//     // code
//     uint8_t crc = modules::crc::CRC8::CCITT_updateCX(0, src[0]);
//     // scan hex value
//     uint8_t rqValue = std::stoul(src.data() + 1, nullptr, 16);
//     crc = modules::crc::CRC8::CCITT_updateCX(crc, rqValue);
//     // [2] is a space
//     // value follows immediately
//     uint16_t paramValue = std::stoul(src.data() + 3, nullptr, 16);
//     crc = modules::crc::CRC8::CCITT_updateW(crc, paramValue);
//     return AppendCRC(src, crc);
// }
