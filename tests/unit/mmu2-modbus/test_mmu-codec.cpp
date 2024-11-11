#include <puppies/xbuddy_extension.hpp>
#include "../../../lib/Marlin/Marlin/src/feature/prusa/MMU2/protocol_logic.h"

#include <catch2/catch.hpp>

namespace MMU2 {
void LogResponseMsg(const char *) {}
} // namespace MMU2

namespace buddy::puppies {
PuppyModbus puppyModbus;

ModbusDevice::ModbusDevice(buddy::puppies::PuppyModbus &bus, unsigned char)
    : bus(bus) {
}

PuppyModbus::PuppyModbus() {
}

uint16_t returnedRead = 0;
CommunicationStatus returnedStatus = CommunicationStatus::OK;

CommunicationStatus PuppyModbus::read_holding(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    REQUIRE(count == 1); // Current mock implementation doesn't work for more, if your tests need them, you need to improve this
    *data = returnedRead;
    return returnedStatus;
}

CommunicationStatus PuppyModbus::write_holding(uint8_t unit, const uint16_t *data, uint16_t count, uint16_t address, bool &dirty) {
    REQUIRE(count == 1); // Current mock implementation doesn't work for more, if your tests need them, you need to improve this
    return returnedStatus;
}

uint16_t returnedQuery[3] = { 0, 0, 0 };

CommunicationStatus PuppyModbus::read_input(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, RequestTiming *const timing, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    switch (address) {
    case XBuddyExtension::mmuCommandInProgressRegisterAddress:
        data[0] = returnedQuery[0];
        data[1] = returnedQuery[1];
        data[2] = returnedQuery[2];
        return returnedStatus;
    default:
        break; // do nothing?
    }
    return returnedStatus;
}

} // namespace buddy::puppies

using namespace buddy::puppies;
using namespace modules::protocol;

void CheckReadRegister(uint8_t address, uint16_t expectedRead) {
    RequestMsg rq(RequestMsgCodes::Read, address);

    xbuddy_extension.post_read_mmu_register(address);

    // check the control structures
    CHECK(xbuddy_extension.mmuModbusRq.rw == XBuddyExtension::MMUModbusRequest::RW::read);
    CHECK(xbuddy_extension.mmuModbusRq.u.read.address == address);

    // prepare simulated modbus comm
    returnedRead = expectedRead;
    returnedStatus = CommunicationStatus::OK;

    // process the message
    CHECK(xbuddy_extension.refresh_mmu() == returnedStatus);

    // check control structures
    CHECK(xbuddy_extension.mmuModbusRq.u.read.value == returnedRead);
    CHECK(xbuddy_extension.mmuModbusRq.u.read.accepted == true);

    // now run ExpectingMessage a couple of times to make sure a valid response got recoded into MMU protocol messages
    ResponseMsg rsp(RequestMsg(RequestMsgCodes::unknown, 0), ResponseMsgParamCodes::unknown, 0);
    uint8_t rawMsg[Protocol::MaxResponseSize()];
    uint8_t rawMsgLen = 0;
    MMU2::ProtocolLogic::ExpectingMessage2(xbuddy_extension.mmuModbusRq, xbuddy_extension.mmuQuery, rsp, rq, rawMsg, rawMsgLen);

    if (address < 4) {
        CHECK(rsp.request.code == RequestMsgCodes::Version);
    } else {
        CHECK(rsp.request.code == RequestMsgCodes::Read);
    }
    CHECK(rsp.request.value == address);
    CHECK(rsp.request.value2 == 0);
    CHECK(rsp.paramCode == ResponseMsgParamCodes::Accepted);
    CHECK(rsp.paramValue == expectedRead);
}

TEST_CASE("MMU2-MODBUS read register") {
    for (uint16_t address = 0; address < 256; ++address) {
        CheckReadRegister(address, 256 - address);
    }
}

void CheckWriteRegister(uint8_t address, uint16_t value) {
    RequestMsg rq(RequestMsgCodes::Write, address, value);
    xbuddy_extension.post_write_mmu_register(address, value);

    // check the control structures
    CHECK(xbuddy_extension.mmuModbusRq.rw == XBuddyExtension::MMUModbusRequest::RW::write);
    CHECK(xbuddy_extension.mmuModbusRq.u.write.address == address);
    CHECK(xbuddy_extension.mmuModbusRq.u.write.value == value);

    // prepare simulated modbus comm
    returnedStatus = CommunicationStatus::OK;

    // process the message
    CHECK(xbuddy_extension.refresh_mmu() == returnedStatus);

    // check control structures
    CHECK(xbuddy_extension.mmuModbusRq.u.write.accepted == true);

    // now run ExpectingMessage a couple of times to make sure a valid response got recoded into MMU protocol messages
    ResponseMsg rsp(RequestMsg(RequestMsgCodes::unknown, 0), ResponseMsgParamCodes::unknown, 0);
    uint8_t rawMsg[Protocol::MaxResponseSize()];
    uint8_t rawMsgLen = 0;
    MMU2::ProtocolLogic::ExpectingMessage2(xbuddy_extension.mmuModbusRq, xbuddy_extension.mmuQuery, rsp, rq, rawMsg, rawMsgLen);

    CHECK(rsp.request.code == RequestMsgCodes::Write);
    CHECK(rsp.request.value == address);
    CHECK(rsp.paramCode == ResponseMsgParamCodes::Accepted);
    CHECK(rsp.paramValue == value);
}

TEST_CASE("MMU2-MODBUS write register") {
    for (uint16_t address = 0; address < 256; ++address) {
        CheckWriteRegister(address, 256 - address);
    }
}

constexpr uint16_t MakeUInt16(uint8_t a, uint8_t b) {
    union {
        uint8_t bytes[2];
        uint16_t word;
    } u;
    u.bytes[0] = a;
    u.bytes[1] = b;
    return u.word;
}

void CheckQuery(uint8_t command, uint8_t param, uint16_t commandStatus, uint16_t pec) {
    RequestMsg rq(RequestMsgCodes::Query, 0);
    xbuddy_extension.post_query_mmu();

    CHECK(xbuddy_extension.mmuModbusRq.rw == XBuddyExtension::MMUModbusRequest::RW::query);

    // prepare simulated modbus comm
    returnedStatus = CommunicationStatus::OK;
    returnedQuery[0] = MakeUInt16(command, param);
    returnedQuery[1] = commandStatus;
    returnedQuery[2] = pec;

    // process the message
    CHECK(xbuddy_extension.refresh_mmu() == returnedStatus);

    // check control structures - response registers are located aside from this structure
    CHECK(xbuddy_extension.mmuQuery.value.cip.bytes[0] == command);
    CHECK(xbuddy_extension.mmuQuery.value.cip.bytes[1] == param);
    CHECK(xbuddy_extension.mmuQuery.value.commandStatus == commandStatus);
    CHECK(xbuddy_extension.mmuQuery.value.pec == pec);

    // now run ExpectingMessage a couple of times to make sure a valid response got recoded into MMU protocol messages
    ResponseMsg rsp(RequestMsg(RequestMsgCodes::unknown, 0), ResponseMsgParamCodes::unknown, 0);
    uint8_t rawMsg[Protocol::MaxResponseSize()];
    uint8_t rawMsgLen = 0;
    MMU2::ProtocolLogic::ExpectingMessage2(xbuddy_extension.mmuModbusRq, xbuddy_extension.mmuQuery, rsp, rq, rawMsg, rawMsgLen);

    CHECK(rsp.request.code == (RequestMsgCodes)command);
    CHECK(rsp.request.value == param);
    CHECK(rsp.request.value2 == 0);
    CHECK(rsp.paramCode == (ResponseMsgParamCodes)commandStatus);
    CHECK(rsp.paramValue == pec);
}

TEST_CASE("MMU2-MODBUS query") {
    CheckQuery('T', '0', (uint16_t)ResponseMsgParamCodes::Processing, (uint16_t)ProgressCode::EngagingIdler);
}

void CheckFailedWriteRegister(uint8_t address, uint16_t value) {
    RequestMsg rq(RequestMsgCodes::Write, address, value);
    xbuddy_extension.post_read_mmu_register(address);

    // check the control structures
    CHECK(xbuddy_extension.mmuModbusRq.rw == XBuddyExtension::MMUModbusRequest::RW::write);
    CHECK(xbuddy_extension.mmuModbusRq.u.write.address == address);
    CHECK(xbuddy_extension.mmuModbusRq.u.write.value == value);

    // prepare simulated modbus comm
    returnedStatus = CommunicationStatus::ERROR;

    // process the message
    CHECK(xbuddy_extension.refresh_mmu() == returnedStatus);

    // check control structures
    CHECK(xbuddy_extension.mmuModbusRq.u.write.accepted == false);

    // now run ExpectingMessage a couple of times to make sure a valid response got recoded into MMU protocol messages
    // this should end up into either rejected or timeout - and that needs to be distinguished properly
    ResponseMsg rsp(RequestMsg(RequestMsgCodes::unknown, 0), ResponseMsgParamCodes::unknown, 0);
    uint8_t rawMsg[Protocol::MaxResponseSize()];
    uint8_t rawMsgLen = 0;
    MMU2::ProtocolLogic::ExpectingMessage2(xbuddy_extension.mmuModbusRq, xbuddy_extension.mmuQuery, rsp, rq, rawMsg, rawMsgLen);

    CHECK(rsp.request.code == RequestMsgCodes::Write);
    CHECK(rsp.request.value == address);
    CHECK(rsp.request.value2 == value);
    CHECK(rsp.paramCode == ResponseMsgParamCodes::Rejected);
    CHECK(rsp.paramValue == value);
}
