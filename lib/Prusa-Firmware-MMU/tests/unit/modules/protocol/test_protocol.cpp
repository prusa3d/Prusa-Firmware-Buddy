#include "catch2/catch.hpp"
#include "../logic/error_codes.h"
#include "../logic/progress_codes.h"
#include "protocol.h"

using Catch::Matchers::Equals;

TEST_CASE("protocol::EncodeRequests", "[protocol]") {
    mp::RequestMsgCodes code;
    uint8_t value;
    std::tie(code, value) = GENERATE(
        std::make_tuple(mp::RequestMsgCodes::Button, 0),
        std::make_tuple(mp::RequestMsgCodes::Button, 1),
        std::make_tuple(mp::RequestMsgCodes::Button, 2),
        std::make_tuple(mp::RequestMsgCodes::Cut, 0),
        std::make_tuple(mp::RequestMsgCodes::Eject, 0),
        std::make_tuple(mp::RequestMsgCodes::Finda, 0),
        std::make_tuple(mp::RequestMsgCodes::Load, 0),
        std::make_tuple(mp::RequestMsgCodes::Load, 1),
        std::make_tuple(mp::RequestMsgCodes::Load, 2),
        std::make_tuple(mp::RequestMsgCodes::Load, 3),
        std::make_tuple(mp::RequestMsgCodes::Load, 4),
        std::make_tuple(mp::RequestMsgCodes::Mode, 0),
        std::make_tuple(mp::RequestMsgCodes::Mode, 1),
        std::make_tuple(mp::RequestMsgCodes::Query, 0),
        std::make_tuple(mp::RequestMsgCodes::Reset, 0),
        std::make_tuple(mp::RequestMsgCodes::Tool, 0),
        std::make_tuple(mp::RequestMsgCodes::Tool, 1),
        std::make_tuple(mp::RequestMsgCodes::Tool, 2),
        std::make_tuple(mp::RequestMsgCodes::Tool, 3),
        std::make_tuple(mp::RequestMsgCodes::Tool, 4),
        std::make_tuple(mp::RequestMsgCodes::Unload, 0),
        std::make_tuple(mp::RequestMsgCodes::Version, 0),
        std::make_tuple(mp::RequestMsgCodes::Version, 1),
        std::make_tuple(mp::RequestMsgCodes::Version, 2),
        std::make_tuple(mp::RequestMsgCodes::Wait, 0),
        std::make_tuple(mp::RequestMsgCodes::unknown, 0));

    std::array<uint8_t, 3> txbuff;

    CHECK(mp::Protocol::EncodeRequest(mp::RequestMsg(code, value), txbuff.data()) == 3);
    CHECK(txbuff[0] == (uint8_t)code);
    CHECK(txbuff[1] == value + '0');
    CHECK(txbuff[2] == '\n');
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

        mp::RequestMsg(mp::RequestMsgCodes::Wait, 0));

    auto responseStatus = GENERATE(mp::ResponseMsgParamCodes::Accepted, mp::ResponseMsgParamCodes::Rejected);

    std::array<uint8_t, 8> txbuff;
    uint8_t msglen = mp::Protocol::EncodeResponseCmdAR(requestMsg, responseStatus, txbuff.data());

    CHECK(msglen == 5);
    CHECK(txbuff[0] == (uint8_t)requestMsg.code);
    CHECK(txbuff[1] == requestMsg.value + '0');
    CHECK(txbuff[2] == ' ');
    CHECK(txbuff[3] == (uint8_t)responseStatus);
    CHECK(txbuff[4] == '\n');
}

TEST_CASE("protocol::EncodeResponseReadFINDA", "[protocol]") {
    auto requestMsg = mp::RequestMsg(mp::RequestMsgCodes::Finda, 0);

    uint8_t findaStatus = GENERATE(0, 1);

    std::array<uint8_t, 8> txbuff;
    uint8_t msglen = mp::Protocol::EncodeResponseReadFINDA(requestMsg, findaStatus, txbuff.data());

    CHECK(msglen == 6);
    CHECK(txbuff[0] == (uint8_t)requestMsg.code);
    CHECK(txbuff[1] == requestMsg.value + '0');
    CHECK(txbuff[2] == ' ');
    CHECK(txbuff[3] == (uint8_t)mp::ResponseMsgParamCodes::Accepted);
    CHECK(txbuff[4] == findaStatus + '0');
    CHECK(txbuff[5] == '\n');
}

TEST_CASE("protocol::EncodeResponseVersion", "[protocol]") {
    std::uint8_t versionQueryType = GENERATE(0, 1, 2, 3);
    auto requestMsg = mp::RequestMsg(mp::RequestMsgCodes::Version, versionQueryType);

    auto version = GENERATE(0, 1, 2, 3, 4, 10, 11, 12, 20, 99, 100, 101, 255);

    std::array<uint8_t, 8> txbuff;
    uint8_t msglen = mp::Protocol::EncodeResponseVersion(requestMsg, version, txbuff.data());

    CHECK(msglen <= 8);
    CHECK(txbuff[0] == (uint8_t)requestMsg.code);
    CHECK(txbuff[1] == requestMsg.value + '0');
    CHECK(txbuff[2] == ' ');
    CHECK(txbuff[3] == (uint8_t)mp::ResponseMsgParamCodes::Accepted);

    if (version < 10) {
        CHECK(txbuff[4] == version + '0');
    } else if (version < 100) {
        CHECK(txbuff[4] == version / 10 + '0');
        CHECK(txbuff[5] == version % 10 + '0');
    } else {
        CHECK(txbuff[4] == version / 100 + '0');
        CHECK(txbuff[5] == (version / 10) % 10 + '0');
        CHECK(txbuff[6] == version % 10 + '0');
    }

    CHECK(txbuff[msglen - 1] == '\n');
}

TEST_CASE("protocol::EncodeResponseQueryOperation", "[protocol]") {
    auto requestMsg = GENERATE(
        mp::RequestMsg(mp::RequestMsgCodes::Cut, 0),

        mp::RequestMsg(mp::RequestMsgCodes::Eject, 0),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 1),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 2),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 3),
        mp::RequestMsg(mp::RequestMsgCodes::Eject, 4),

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

        mp::RequestMsg(mp::RequestMsgCodes::Unload, 0),

        mp::RequestMsg(mp::RequestMsgCodes::Wait, 0));

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

    std::array<uint8_t, 10> txbuff;

    uint16_t encodedParamValue = responseStatus == mp::ResponseMsgParamCodes::Error ? (uint16_t)error : (uint16_t)value;

    uint8_t msglen = mp::Protocol::EncodeResponseQueryOperation(requestMsg, responseStatus, encodedParamValue, txbuff.data());

    CHECK(msglen <= txbuff.size());
    CHECK(txbuff[0] == (uint8_t)requestMsg.code);
    CHECK(txbuff[1] == requestMsg.value + '0');
    CHECK(txbuff[2] == ' ');
    CHECK(txbuff[3] == (uint8_t)responseStatus);

    if (responseStatus == mp::ResponseMsgParamCodes::Finished) {
        CHECK(txbuff[4] == '\n');
        CHECK(msglen == 5);
    } else {
        if (encodedParamValue < 10) {
            CHECK(txbuff[4] == encodedParamValue + '0');
        } else if (encodedParamValue < 100) {
            CHECK(txbuff[4] == encodedParamValue / 10 + '0');
            CHECK(txbuff[5] == encodedParamValue % 10 + '0');
        } else if (encodedParamValue < 1000) {
            CHECK(txbuff[4] == encodedParamValue / 100 + '0');
            CHECK(txbuff[5] == (encodedParamValue / 10) % 10 + '0');
            CHECK(txbuff[6] == encodedParamValue % 10 + '0');
        } else if (encodedParamValue < 10000) {
            CHECK(txbuff[4] == encodedParamValue / 1000 + '0');
            CHECK(txbuff[5] == (encodedParamValue / 100) % 10 + '0');
            CHECK(txbuff[6] == (encodedParamValue / 10) % 10 + '0');
            CHECK(txbuff[7] == encodedParamValue % 10 + '0');
        } else {
            CHECK(txbuff[4] == encodedParamValue / 10000 + '0');
            CHECK(txbuff[5] == (encodedParamValue / 1000) % 10 + '0');
            CHECK(txbuff[6] == (encodedParamValue / 100) % 10 + '0');
            CHECK(txbuff[7] == (encodedParamValue / 10) % 10 + '0');
            CHECK(txbuff[8] == encodedParamValue % 10 + '0');
        }

        CHECK(txbuff[msglen - 1] == '\n');
    }
}

TEST_CASE("protocol::DecodeRequest", "[protocol]") {
    mp::Protocol p;
    const char *rxbuff = GENERATE(
        "B0\n", "B1\n", "B2\n",
        "E0\n", "E1\n", "E2\n", "E3\n", "E4\n",
        "F0\n", "F1\n",
        "f0\n", "f1\n",
        "K0\n",
        "L0\n", "L1\n", "L2\n", "L3\n", "L4\n",
        "M0\n", "M1\n",
        "P0\n",
        "Q0\n",
        "S0\n", "S1\n", "S2\n", "S3\n",
        "T0\n", "T1\n", "T2\n", "T3\n",
        "U0\n",
        "W0\n",
        "X0\n");

    const char *pc = rxbuff;
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
}

TEST_CASE("protocol::DecodeResponseReadFinda", "[protocol]") {
    mp::Protocol p;
    const char *rxbuff = GENERATE(
        "P0 A0\n",
        "P0 A1\n");

    const char *pc = rxbuff;
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

TEST_CASE("protocol::DecodeResponseQueryOperation", "[protocol]") {
    mp::Protocol p;
    const char *cmdReference = GENERATE(
        "E0", "E1", "E2", "E3", "E4",
        "K0", "K1", "K2", "K3", "K4",
        "L0", "L1", "L2", "L3", "L4",
        "T0", "T1", "T2", "T3", "T4",
        "U0",
        "W0");

    const char *status = GENERATE(
        "P0", "P1", "E0", "E1", "E9", "F");

    std::string rxbuff(cmdReference);
    rxbuff += ' ';
    rxbuff += status;
    rxbuff += '\n';

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
    if ((uint8_t)rsp.paramCode != (uint8_t)mp::ResponseMsgParamCodes::Finished) {
        CHECK((uint8_t)rsp.paramValue == rxbuff[4] - '0');
    }
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

// Beware - this test makes 18M+ combinations, run only when changing the implementation of the codec
// Therefore it is disabled [.] by default
TEST_CASE("protocol::DecodeResponseErrorsCross", "[protocol][.]") {
    mp::Protocol p;

    const char *validInitialSpaces = "";
    const char *invalidInitialSpaces = GENERATE(" ", "  ");
    bool viInitialSpace = GENERATE(true, false);

    const char *validReqCode = GENERATE("B", "E", "F", "f", "K", "L", "M", "P", "Q", "S", "T", "U", "W", "X");
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
