#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "../logic/error_codes.h"
#include "../logic/progress_codes.h"
#include "protocol.h"
#include <array>
#include <algorithm>

// some safe margin for the buffer
using TXBuff = std::array<uint8_t, 2 * std::max(mp::Protocol::MaxRequestSize(), mp::Protocol::MaxResponseSize())>;

TEST_CASE("protocol::Char2Nibble2Char", "[protocol]") {
    uint8_t character, value;
    std::tie(character, value) = GENERATE(
        std::make_tuple('0', 0),
        std::make_tuple('1', 1),
        std::make_tuple('2', 2),
        std::make_tuple('3', 3),
        std::make_tuple('4', 4),
        std::make_tuple('5', 5),
        std::make_tuple('6', 6),
        std::make_tuple('7', 7),
        std::make_tuple('8', 8),
        std::make_tuple('9', 9),
        std::make_tuple('a', 0xa),
        std::make_tuple('b', 0xb),
        std::make_tuple('c', 0xc),
        std::make_tuple('d', 0xd),
        std::make_tuple('e', 0xe),
        std::make_tuple('f', 0xf),
        std::make_tuple('g', 0) // invalid character defaults to 0
    );
    REQUIRE(mp::Protocol::Char2Nibble(character) == value);
    if (character != 'g') { // skip the invalid char
        REQUIRE(mp::Protocol::Nibble2Char(value) == character);
    }
}

TEST_CASE("protocol::Value2Hex", "[protocol]") {
    for (uint32_t v = 0; v < 0xffff; ++v) {
        constexpr size_t buffSize = 5;
        uint8_t tmp[buffSize];
        uint8_t chars = mp::Protocol::UInt16ToHex((uint16_t)v, tmp);
        if (v < 0x10) {
            REQUIRE(chars == 1);
        } else if (v < 0x100) {
            REQUIRE(chars == 2);
        } else if (v < 0x1000) {
            REQUIRE(chars == 3);
        } else if (v < 0x10000) {
            REQUIRE(chars == 4);
        }
        std::string tmps(tmp, tmp + chars);
        REQUIRE(std::stoul(tmps, nullptr, 16) == v);
    }
}

std::string MakeCRC(uint8_t crc) {
    char tmp[5]; // *ffn are 4 bytes + \0 at the end
    snprintf(tmp, 5, "*%x\n", (unsigned)crc);
    return std::string(tmp);
}

std::string MakeCRC(const std::string_view src) {
    // this code basically needs parsing of the input text and compute the CRC from the parsed data
    REQUIRE(src.size() > 1);
    // code
    uint8_t crc = modules::crc::CRC8::CCITT_updateCX(0, src[0]);
    // scan hex value
    uint8_t rqValue = std::stoul(src.data() + 1, nullptr, 16);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, rqValue);
    crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
    if (!src[2])
        return MakeCRC(crc); // eof
    // [2] is a space
    REQUIRE(src.size() > 3);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, src[3]); // param code
    if (!src[4])
        return MakeCRC(crc); // eof
    REQUIRE(src.size() > 4);
    uint16_t paramValue = std::stoul(src.data() + 4, nullptr, 16);
    crc = modules::crc::CRC8::CCITT_updateW(crc, paramValue);
    return MakeCRC(crc);
}

TEST_CASE("protocol::EncodeRequests", "[protocol]") {
    mp::RequestMsgCodes code;
    uint8_t value;
    std::string str;
    std::tie(code, value, str) = GENERATE(
        std::make_tuple(mp::RequestMsgCodes::Button, 0, "B0"),
        std::make_tuple(mp::RequestMsgCodes::Button, 1, "B1"),
        std::make_tuple(mp::RequestMsgCodes::Button, 2, "B2"),
        std::make_tuple(mp::RequestMsgCodes::Cut, 0, "K0"),
        std::make_tuple(mp::RequestMsgCodes::Eject, 0, "E0"),
        std::make_tuple(mp::RequestMsgCodes::Finda, 0, "P0"),
        std::make_tuple(mp::RequestMsgCodes::Home, 0, "H0"),
        std::make_tuple(mp::RequestMsgCodes::Load, 0, "L0"),
        std::make_tuple(mp::RequestMsgCodes::Load, 1, "L1"),
        std::make_tuple(mp::RequestMsgCodes::Load, 2, "L2"),
        std::make_tuple(mp::RequestMsgCodes::Load, 3, "L3"),
        std::make_tuple(mp::RequestMsgCodes::Load, 4, "L4"),
        std::make_tuple(mp::RequestMsgCodes::Mode, 0, "M0"),
        std::make_tuple(mp::RequestMsgCodes::Mode, 1, "M1"),
        std::make_tuple(mp::RequestMsgCodes::Query, 0, "Q0"),
        std::make_tuple(mp::RequestMsgCodes::Reset, 0, "X0"),
        std::make_tuple(mp::RequestMsgCodes::Tool, 0, "T0"),
        std::make_tuple(mp::RequestMsgCodes::Tool, 1, "T1"),
        std::make_tuple(mp::RequestMsgCodes::Tool, 2, "T2"),
        std::make_tuple(mp::RequestMsgCodes::Tool, 3, "T3"),
        std::make_tuple(mp::RequestMsgCodes::Tool, 4, "T4"),
        std::make_tuple(mp::RequestMsgCodes::Unload, 0, "U0"),
        std::make_tuple(mp::RequestMsgCodes::Version, 0, "S0"),
        std::make_tuple(mp::RequestMsgCodes::Version, 1, "S1"),
        std::make_tuple(mp::RequestMsgCodes::Version, 2, "S2")
        /*std::make_tuple(mp::RequestMsgCodes::unknown, 0, "\00")*/);

    TXBuff txbuff;
    mp::RequestMsg msg(code, value);
    str.append(MakeCRC(msg.CRC()));

    uint8_t packetSize = mp::Protocol::EncodeRequest(msg, txbuff.data());
    REQUIRE(packetSize < mp::Protocol::MaxRequestSize());
    CHECK(packetSize <= 6);
    for (uint8_t i = 0; i < packetSize; ++i) {
        CHECK(str[i] == txbuff[i]);
    }
}

TEST_CASE("protocol::EncodeResponseCmdAR", "[protocol]") {
    auto requestMsg = GENERATE(
        mp::RequestMsg(mp::RequestMsgCodes::Button, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Button, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Button, 2),

        mp::RequestMsg(mp::RequestMsgCodes::Cut, 0),

        mp::RequestMsg(mp::RequestMsgCodes::Eject, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 2),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 3),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 4),

        mp::RequestMsg(mp::RequestMsgCodes::Home, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Home, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Home, 2),

        mp::RequestMsg(mp::RequestMsgCodes::FilamentType, 0),
        mp::RequestMsg(mp::RequestMsgCodes::FilamentType, 1),

        mp::RequestMsg(mp::RequestMsgCodes::FilamentSensor, 0),
        mp::RequestMsg(mp::RequestMsgCodes::FilamentSensor, 1),

        mp::RequestMsg(mp::RequestMsgCodes::Load, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 2),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 3),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 4),

        mp::RequestMsg(mp::RequestMsgCodes::Mode, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Mode, 1),

        mp::RequestMsg(mp::RequestMsgCodes::Tool, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 2),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 3),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 4),

        mp::RequestMsg(mp::RequestMsgCodes::Unload, 0),

        mp::RequestMsg(mp::RequestMsgCodes::Write, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Write, 9),
        mp::RequestMsg(mp::RequestMsgCodes::Write, 0xa),
        mp::RequestMsg(mp::RequestMsgCodes::Write, 0xf),
        mp::RequestMsg(mp::RequestMsgCodes::Write, 10),
        mp::RequestMsg(mp::RequestMsgCodes::Write, 19),
        mp::RequestMsg(mp::RequestMsgCodes::Write, 0xfa),
        mp::RequestMsg(mp::RequestMsgCodes::Write, 0xff));

    auto responseStatus = GENERATE(mp::ResponseMsgParamCodes::Accepted, mp::ResponseMsgParamCodes::Rejected, mp::ResponseMsgParamCodes::Button);

    TXBuff txbuff;
    mp::ResponseMsg rsp(requestMsg, responseStatus, 0);
    uint8_t msglen = mp::Protocol::EncodeResponseCmdAR(requestMsg, responseStatus, txbuff.data());
    REQUIRE(msglen < mp::Protocol::MaxResponseSize());
    std::string crc = MakeCRC(rsp.CRC());

    if (requestMsg.value < 10) {
        CHECK(msglen == 4 + crc.size());
        CHECK(txbuff[0] == (uint8_t)requestMsg.code);
        CHECK(txbuff[1] == requestMsg.value + '0');
        CHECK(txbuff[2] == ' ');
        CHECK(txbuff[3] == (uint8_t)responseStatus);
        CHECK(txbuff[4] == '*');
        CHECK(txbuff[msglen - 1] == '\n');
    } else if (requestMsg.value < 16) {
        CHECK(msglen == 4 + crc.size());
        CHECK(txbuff[0] == (uint8_t)requestMsg.code);
        CHECK(txbuff[1] == requestMsg.value - 10 + 'a');
        CHECK(txbuff[2] == ' ');
        CHECK(txbuff[3] == (uint8_t)responseStatus);
        CHECK(txbuff[4] == '*');
        CHECK(txbuff[msglen - 1] == '\n');
    } else if (requestMsg.value < 0x1a) {
        CHECK(msglen == 5 + crc.size());
        CHECK(txbuff[0] == (uint8_t)requestMsg.code);
        CHECK(txbuff[1] == (requestMsg.value >> 4U) + '0');
        CHECK(txbuff[2] == (requestMsg.value & 0xfU) + '0');
        CHECK(txbuff[3] == ' ');
        CHECK(txbuff[4] == (uint8_t)responseStatus);
        CHECK(txbuff[5] == '*');
        CHECK(txbuff[msglen - 1] == '\n');
    } else {
        CHECK(msglen == 5 + crc.size());
        CHECK(txbuff[0] == (uint8_t)requestMsg.code);
        CHECK(txbuff[1] == (requestMsg.value >> 4U) - 10 + 'a');
        CHECK(txbuff[2] == (requestMsg.value & 0xfU) - 10 + 'a');
        CHECK(txbuff[3] == ' ');
        CHECK(txbuff[4] == (uint8_t)responseStatus);
        CHECK(txbuff[5] == '*');
        CHECK(txbuff[msglen - 1] == '\n');
    }
}

TEST_CASE("protocol::EncodeResponseReadFINDA", "[protocol]") {
    auto requestMsg = mp::RequestMsg(mp::RequestMsgCodes::Finda, 0);
    uint8_t findaStatus = GENERATE(0, 1);

    TXBuff txbuff;
    mp::ResponseMsg rsp(requestMsg, mp::ResponseMsgParamCodes::Accepted, findaStatus);
    std::string crc = MakeCRC(rsp.CRC());
    uint8_t msglen = mp::Protocol::EncodeResponseReadFINDA(requestMsg, findaStatus, txbuff.data());
    REQUIRE(msglen < mp::Protocol::MaxResponseSize());

    CHECK(msglen == 5 + crc.size());
    CHECK(txbuff[0] == (uint8_t)requestMsg.code);
    CHECK(txbuff[1] == requestMsg.value + '0');
    CHECK(txbuff[2] == ' ');
    CHECK(txbuff[3] == (uint8_t)mp::ResponseMsgParamCodes::Accepted);
    CHECK(txbuff[4] == findaStatus + '0');
    CHECK(txbuff[5] == '*');
    CHECK(txbuff[msglen - 1] == '\n');
}

TEST_CASE("protocol::EncodeResponseVersion", "[protocol]") {
    std::uint8_t versionQueryType = GENERATE(0, 1, 2, 3);
    auto requestMsg = mp::RequestMsg(mp::RequestMsgCodes::Version, versionQueryType);

    for (uint32_t version = 0; version < 0xffff; ++version) {
        TXBuff txbuff;
        mp::ResponseMsg rsp(requestMsg, mp::ResponseMsgParamCodes::Accepted, (uint16_t)version);
        uint8_t msglen = mp::Protocol::EncodeResponseRead(requestMsg, true, (uint16_t)version, txbuff.data());
        REQUIRE(msglen < mp::Protocol::MaxResponseSize());
        std::string crc = MakeCRC(rsp.CRC());

        CHECK(txbuff[0] == (uint8_t)requestMsg.code);
        CHECK(txbuff[1] == requestMsg.value + '0');
        CHECK(txbuff[2] == ' ');
        CHECK(txbuff[3] == (uint8_t)mp::ResponseMsgParamCodes::Accepted);

        char chk[10];
        int chars = snprintf(chk, sizeof(chk), "%x", version);
        REQUIRE(chars < 10);
        std::string chks(chk, chk + chars);
        chks.append(crc);
        std::string vers((const char *)(&txbuff[4]), (const char *)(&txbuff[msglen]));

        REQUIRE(chks == vers);

        CHECK(txbuff[msglen - 1] == '\n');
    }
}

TEST_CASE("protocol::EncodeResponseQueryOperation", "[protocol]") {
    auto requestMsg = GENERATE(
        mp::RequestMsg(mp::RequestMsgCodes::Cut, 0),

        mp::RequestMsg(mp::RequestMsgCodes::Eject, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 2),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 3),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 4),

        mp::RequestMsg(mp::RequestMsgCodes::Home, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Home, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Home, 2),

        mp::RequestMsg(mp::RequestMsgCodes::Load, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 2),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 3),
        mp::RequestMsg(mp::RequestMsgCodes::Load, 4),

        mp::RequestMsg(mp::RequestMsgCodes::Tool, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 2),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 3),
        mp::RequestMsg(mp::RequestMsgCodes::Tool, 4),

        mp::RequestMsg(mp::RequestMsgCodes::Unload, 0));

    auto responseStatus = GENERATE(mp::ResponseMsgParamCodes::Processing, mp::ResponseMsgParamCodes::Error, mp::ResponseMsgParamCodes::Finished);

    auto value = GENERATE(
        ProgressCode::OK,
        ProgressCode::EngagingIdler,
        ProgressCode::DisengagingIdler,
        ProgressCode::UnloadingToFinda,
        ProgressCode::UnloadingToPulley,
        ProgressCode::FeedingToFinda,
        ProgressCode::FeedingToBondtech,
        ProgressCode::AvoidingGrind,
        ProgressCode::FinishingMoves,
        ProgressCode::ERRDisengagingIdler,
        ProgressCode::ERREngagingIdler,
        ProgressCode::ERRWaitingForUser,
        ProgressCode::ERRInternal,
        ProgressCode::ERRHelpingFilament,
        ProgressCode::ERRTMCFailed,
        ProgressCode::UnloadingFilament,
        ProgressCode::LoadingFilament,
        ProgressCode::SelectingFilamentSlot,
        ProgressCode::PreparingBlade,
        ProgressCode::PushingFilament,
        ProgressCode::PerformingCut,
        ProgressCode::ReturningSelector,
        ProgressCode::ParkingSelector,
        ProgressCode::EjectingFilament);

    auto error = GENERATE(
        ErrorCode::RUNNING,
        ErrorCode::OK,
        ErrorCode::FINDA_DIDNT_SWITCH_ON,
        ErrorCode::FINDA_DIDNT_SWITCH_OFF,
        ErrorCode::FSENSOR_DIDNT_SWITCH_ON,
        ErrorCode::FSENSOR_DIDNT_SWITCH_OFF,
        ErrorCode::FILAMENT_ALREADY_LOADED,
        ErrorCode::MMU_NOT_RESPONDING,
        ErrorCode::INTERNAL,
        ErrorCode::TMC_PULLEY_BIT,
        ErrorCode::TMC_SELECTOR_BIT,
        ErrorCode::TMC_IDLER_BIT,
        ErrorCode::TMC_IOIN_MISMATCH,
        ErrorCode::TMC_RESET,
        ErrorCode::TMC_UNDERVOLTAGE_ON_CHARGE_PUMP,
        ErrorCode::TMC_SHORT_TO_GROUND,
        ErrorCode::TMC_OVER_TEMPERATURE_WARN,
        ErrorCode::TMC_OVER_TEMPERATURE_ERROR);

    TXBuff txbuff;

    uint16_t encodedParamValue = responseStatus == mp::ResponseMsgParamCodes::Error ? (uint16_t)error : (uint16_t)value;

    mp::ResponseMsg rsp(requestMsg, mp::ResponseMsgParamCodes(responseStatus), encodedParamValue);
    uint8_t msglen = mp::Protocol::EncodeResponseQueryOperation(requestMsg, mp::ResponseCommandStatus(responseStatus, encodedParamValue), txbuff.data());
    REQUIRE(msglen < mp::Protocol::MaxResponseSize());

    CHECK(msglen <= txbuff.size());
    CHECK(txbuff[0] == (uint8_t)requestMsg.code);
    CHECK(txbuff[1] == requestMsg.value + '0');
    CHECK(txbuff[2] == ' ');
    CHECK(txbuff[3] == (uint8_t)responseStatus);

    char chk[6];
    int chars = snprintf(chk, 6, "%x", encodedParamValue);
    REQUIRE(chars < 6);
    std::string chks(chk, chk + chars);
    chks.append(MakeCRC(rsp.ComputeCRC8()));
    std::string txs((const char *)(&txbuff[4]), (const char *)(&txbuff[msglen]));
    REQUIRE(chks == txs);
    CHECK(txbuff[msglen - 1] == '\n');
}

TEST_CASE("protocol::DecodeRequest", "[protocol]") {
    mp::Protocol p;
    std::string rxbuff = GENERATE(
        "B0", "B1", "B2",
        "E0", "E1", "E2", "E3", "E4",
        "H0", "H1", "H2",
        "F0", "F1",
        "f0", "f1",
        "K0",
        "L0", "L1", "L2", "L3", "L4",
        "M0", "M1",
        "P0",
        "Q0",
        "S0", "S1", "S2", "S3",
        "T0", "T1", "T2", "T3", "T4",
        "U0",
        "X0");

    rxbuff.append(MakeCRC(rxbuff.c_str()));

    const char *pc = rxbuff.c_str();
    for (;;) {
        uint8_t c = *pc++;
        if (c == 0) {
            // end of input test data
            break;
        } else if (c == '\n') {
            // regular end of message line
            CHECK(p.DecodeRequest(c) == mp::DecodeStatus::MessageCompleted);
        } else {
            CHECK(p.DecodeRequest(c) == mp::DecodeStatus::NeedMoreData);
        }
    }

    // check the message type
    const mp::RequestMsg &rq = p.GetRequestMsg();
    CHECK((uint8_t)rq.code == rxbuff[0]);
    CHECK(rq.value == rxbuff[1] - '0');
    // CRC is checked implicitly - the protocol would not return MessageCompleted if CRC mismatched
}

TEST_CASE("protocol::DecodeResponseReadFinda", "[protocol]") {
    mp::Protocol p;
    std::string rxbuff = GENERATE(
        "P0 A0",
        "P0 A1");
    rxbuff.append(MakeCRC(rxbuff.c_str()));
    const char *pc = rxbuff.c_str();
    for (;;) {
        uint8_t c = *pc++;
        if (c == 0) {
            // end of input test data
            break;
        } else if (c == '\n') {
            // regular end of message line
            CHECK(p.DecodeResponse(c) == mp::DecodeStatus::MessageCompleted);
        } else {
            CHECK(p.DecodeResponse(c) == mp::DecodeStatus::NeedMoreData);
        }
    }

    // check the message type
    const mp::ResponseMsg &rsp = p.GetResponseMsg();
    CHECK((uint8_t)rsp.request.code == rxbuff[0]);
    CHECK(rsp.request.value == rxbuff[1] - '0');
    CHECK((uint8_t)rsp.paramCode == rxbuff[3]);
    CHECK((uint8_t)rsp.paramValue == rxbuff[4] - '0');
    // CRC is checked implicitly - the protocol would not return MessageCompleted if CRC mismatched
}

TEST_CASE("protocol::DecodeResponseQueryOperation", "[protocol][.]") {
    mp::Protocol p;
    const char *cmdReference = GENERATE(
        "E0", "E1", "E2", "E3", "E4",
        "H0", "H1", "H2",
        "K0", "K1", "K2", "K3", "K4",
        "L0", "L1", "L2", "L3", "L4",
        "T0", "T1", "T2", "T3", "T4",
        "U0");

    const char *status = GENERATE(
        "P0", "P1", "E0", "E1", "E9", "F", "B0", "B1", "B2");

    std::string rxbuff(cmdReference);
    rxbuff += ' ';
    rxbuff += status;

    rxbuff.append(MakeCRC(rxbuff.c_str()));

    const char *pc = rxbuff.c_str();
    for (;;) {
        uint8_t c = *pc++;
        if (c == 0) {
            // end of input test data
            break;
        } else if (c == '\n') {
            // regular end of message line
            CHECK(p.DecodeResponse(c) == mp::DecodeStatus::MessageCompleted);
        } else {
            CHECK(p.DecodeResponse(c) == mp::DecodeStatus::NeedMoreData);
        }
    }

    // check the message type
    const mp::ResponseMsg &rsp = p.GetResponseMsg();
    CHECK((uint8_t)rsp.request.code == rxbuff[0]);
    CHECK(rsp.request.value == rxbuff[1] - '0');
    CHECK((uint8_t)rsp.paramCode == rxbuff[3]);
    CHECK((uint8_t)rsp.paramValue == rxbuff[4] - '0');
}

TEST_CASE("protocol::DecodeRequestErrors", "[protocol]") {
    mp::Protocol p;
    const char b0[] = "b0";
    CHECK(p.DecodeRequest(b0[0]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b0[1]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);

    // reset protokol decoder
    CHECK(p.DecodeRequest('\n') == mp::DecodeStatus::MessageCompleted);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);

    const char B1_[] = "B1 \n";
    CHECK(p.DecodeRequest(B1_[0]) == mp::DecodeStatus::NeedMoreData);
    CHECK(p.DecodeRequest(B1_[1]) == mp::DecodeStatus::NeedMoreData);
    CHECK(p.DecodeRequest(B1_[2]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(B1_[3]) == mp::DecodeStatus::MessageCompleted);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);

    const char _B2[] = " B2\n";
    CHECK(p.DecodeRequest(_B2[0]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(_B2[1]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(_B2[2]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(_B2[3]) == mp::DecodeStatus::MessageCompleted);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);

    const char _B0_[] = " B0 ";
    CHECK(p.DecodeRequest(_B0_[0]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(_B0_[1]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(_B0_[2]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(_B0_[3]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest('\n') == mp::DecodeStatus::MessageCompleted);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);

    const char _B0_bad_crc[] = "B0*aa\n";
    CHECK(p.DecodeRequest(_B0_bad_crc[0]) == mp::DecodeStatus::NeedMoreData);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::Button);
    CHECK(p.DecodeRequest(_B0_bad_crc[1]) == mp::DecodeStatus::NeedMoreData);
    CHECK(p.DecodeRequest(_B0_bad_crc[2]) == mp::DecodeStatus::NeedMoreData);
    CHECK(p.DecodeRequest(_B0_bad_crc[3]) == mp::DecodeStatus::NeedMoreData);
    CHECK(p.DecodeRequest(_B0_bad_crc[4]) == mp::DecodeStatus::NeedMoreData);
    CHECK(p.DecodeRequest(_B0_bad_crc[5]) == mp::DecodeStatus::Error); // bad crc
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
}

TEST_CASE("protocol::DecodeResponseErrors", "[protocol]") {
    mp::Protocol p;

    const char b0[] = "b0 A\n";
    CHECK(p.DecodeRequest(b0[0]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b0[1]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b0[2]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b0[3]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b0[4]) == mp::DecodeStatus::MessageCompleted);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);

    const char b1[] = "b0A\n";
    CHECK(p.DecodeRequest(b1[0]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b1[1]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b1[2]) == mp::DecodeStatus::Error);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
    CHECK(p.DecodeRequest(b1[3]) == mp::DecodeStatus::MessageCompleted);
    CHECK(p.GetRequestMsg().code == mp::RequestMsgCodes::unknown);
}

TEST_CASE("protocol::WriteRequest", "[protocol]") {
    // write requests need special handling
    TXBuff txbuff;
    mp::Protocol p;
    for (uint8_t address = 0; address < 255; ++address) {
        for (uint32_t value2 = 2; value2 < 0x10000; ++value2) {
            uint8_t msglen = mp::Protocol::EncodeWriteRequest(address, (uint16_t)value2, txbuff.data());
            REQUIRE(msglen < mp::Protocol::MaxRequestSize());

            p.ResetRequestDecoder();
            for (uint8_t i = 0; i < msglen; ++i) {
                p.DecodeRequest(txbuff[i]);
            }

            REQUIRE(p.requestMsg.code == mp::RequestMsgCodes::Write);
            REQUIRE(p.requestMsg.value == address);
            REQUIRE(p.requestMsg.value2 == (uint16_t)value2);
        }
    }
}

TEST_CASE("protocol::ReadRequest", "[protocol]") {
    TXBuff txbuff;
    mp::Protocol p;
    for (uint16_t address = 0; address <= 255; ++address) {
        mp::RequestMsg msg(mp::RequestMsgCodes::Read, (uint8_t)address);
        uint8_t msglen = mp::Protocol::EncodeRequest(msg, txbuff.data());
        REQUIRE(msglen < mp::Protocol::MaxRequestSize());

        p.ResetRequestDecoder();
        for (uint8_t i = 0; i < msglen; ++i) {
            p.DecodeRequest(txbuff[i]);
        }

        REQUIRE(p.requestMsg.code == mp::RequestMsgCodes::Read);
        REQUIRE(p.requestMsg.value == (uint8_t)address);
    }
}

TEST_CASE("protocol::ReadResponse", "[protocol]") {
    TXBuff txbuff;
    mp::Protocol p;
    for (uint8_t address = 0; address < 255; ++address) {
        for (uint32_t value2 = 2; value2 < 0x10000; ++value2) {
            for (uint8_t ar = 0; ar <= 1; ++ar) {
                mp::RequestMsg msg(mp::RequestMsgCodes::Read, address);
                uint8_t msglen = mp::Protocol::EncodeResponseRead(msg, ar != 0, value2, txbuff.data());
                REQUIRE(msglen < mp::Protocol::MaxResponseSize());

                p.ResetResponseDecoder();
                for (uint8_t i = 0; i < msglen; ++i) {
                    p.DecodeResponse(txbuff[i]);
                }

                REQUIRE(p.responseMsg.request.code == mp::RequestMsgCodes::Read);
                REQUIRE(p.responseMsg.request.value == address);
                if (ar) {
                    REQUIRE(p.responseMsg.paramCode == mp::ResponseMsgParamCodes::Accepted);
                    REQUIRE(p.responseMsg.paramValue == value2);
                } else {
                    REQUIRE(p.responseMsg.paramCode == mp::ResponseMsgParamCodes::Rejected);
                    REQUIRE(p.responseMsg.paramValue == 0);
                }
            }
        }
    }
}

// Beware - this test makes 18M+ combinations, run only when changing the implementation of the codec
// Therefore it is disabled [.] by default
TEST_CASE("protocol::DecodeResponseErrorsCross", "[protocol][.]") {
    mp::Protocol p;

    const char *validInitialSpaces = "";
    const char *invalidInitialSpaces = GENERATE(" ", "  ");
    bool viInitialSpace = GENERATE(true, false);

    const char *validReqCode = GENERATE("B", "E", "H", "F", "f", "K", "L", "M", "P", "Q", "S", "T", "U", "X");
    const char *invalidReqCode = GENERATE("A", "R");
    bool viReqCode = GENERATE(true, false);

    const char *validReqValue = GENERATE("0", "1", "2", "3", "4");
    // these are silently accepted
    //    const char *invalidReqValue = GENERATE(/*"5", */"10", "100");
    //    bool viReqValue = GENERATE(true, false);

    const char *validSpace = " ";
    const char *invalidSpace = GENERATE("", "  ");
    bool viSpace = GENERATE(true, false);

    const char *validRspCode = GENERATE("A", "R", "P", "E", "F");
    const char *invalidRspCode = GENERATE("B", "K", "L", "M", "Q");
    bool viRspCode = GENERATE(true, false);

    const char *validRspValue = GENERATE("0", "1", "2", "3", "10", "11", "100", "255");

    const char *validTerminatingSpaces = "";
    const char *invalidTerminatingSpaces = GENERATE(" ", "  ");
    bool viTerminatingSpaces = GENERATE(true, false);

    // skip valid combinations
    std::string msg;
    msg += viInitialSpace ? validInitialSpaces : invalidInitialSpaces;
    msg += viReqCode ? validReqCode : invalidReqCode;
    msg += validReqValue; //viReqValue ? validReqValue : invalidReqValue;
    msg += viSpace ? validSpace : invalidSpace;
    const char *rspCode = viRspCode ? validRspCode : invalidRspCode;
    msg += rspCode;
    if (rspCode[0] == 'F') {
        // this one doesn't have any value behind
    } else {
        msg += validRspValue;
    }
    msg += viTerminatingSpaces ? validTerminatingSpaces : invalidTerminatingSpaces;
    msg += '\n';

    bool shouldPass = viInitialSpace && viReqCode && /*viReqValue && */ viSpace && viRspCode && viTerminatingSpaces;
    bool failed = false;
    std::for_each(msg.cbegin(), msg.cend(), [&](uint8_t c) {
        if (p.DecodeResponse(c) == mp::DecodeStatus::Error) {
            failed = true;
        }
    });
    CHECK(failed != shouldPass); // it must have failed!
    if (failed) {
        CHECK(p.GetResponseMsg().paramCode == mp::ResponseMsgParamCodes::unknown);
    }
}
