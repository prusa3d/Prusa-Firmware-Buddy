/// @file
#include "mmu.hpp"

#include "hal.hpp"
#include <freertos/timing.hpp>
#include <xbuddy_extension_shared/mmu_bridge.hpp>

using namespace modules::protocol;
using namespace xbuddy_extension_shared::mmu_bridge;

// #define SIMULATE_MMU
#ifdef SIMULATE_MMU
modbus::Callbacks::Status MMU::read_register(uint8_t, uint16_t address, uint16_t &out) {
    switch (address) {
    case 0: // first registers need to be read via the 'S' query. MMU FW would handle the 'R' query as well, but the bootloader wouldn't
        out = 3;
        break;
    case 1:
        out = 0;
        break;
    case 2:
        out = 3;
        break;
    case 3:
        out = 0;
        break;
    case 4:
    case 8:
    case 0x1a:
    case 0x1b:
    case 0x1c:
        out = 0;
        break;

    case buttonRegisterAddress:
        out = 0; // button is probably not interesting
        break;
    case commandInProgressRegisterAddress:
        out = pack_command('X', 0); // that's the current command in progress
        break;
    case commandStatusRegisterAddress:
        out = 'F'; // for now, let's consider X0 as finished ;)
        break;
    case commandProgressOrErrorCodeRegisterAddress:
        out = 0;
        break;
    default:
        break;
    }
    return Status::Ok;
}

modbus::Callbacks::Status MMU::write_register(uint8_t, uint16_t address, uint16_t) {
    switch (address) {
    case 7:
    case 9:
    case 11:
    case 12:
    case 23:
    case 24:
    case 25:
    case 29:
    case 35:
        return Status::Ok;
    case 4:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 27:
    case 28:
    case 30:
    case 31:
    case 32:
    case 34:
        return Status::Ok;

    case buttonRegisterAddress: { // button register
        return Status::Ok;
    }
    }
    return Status::IllegalAddress;
}
#else

modbus::Callbacks::Status MMU::read_register(uint8_t, uint16_t address, uint16_t &out) {
    uint8_t txbuff[32];
    switch (address) {
    case 0: // first registers need to be read via the 'S' query. MMU FW would handle the 'R' query as well, but the bootloader wouldn't
    case 1:
    case 2:
    case 3: {
        RequestMsg rq(RequestMsgCodes::Version, address);
        uint8_t size = protocol.EncodeRequest(rq, txbuff);
        uart.write(txbuff, size);
        RecordUARTActivity();
        // blocking wait for response
        return WaitForMMUResponse([&]() { out = rsp.paramValue; });
    }

    // virtual query registers
    // we'll get multiple calls per register, but this triplet is being handled as a single msg into the MMU
    // so we have to do some magic here to post just one MMU message but serve 3 consecutive read_registers
    // Beware: the code is optimized for queries, reading individual registers 254 and 255 separately will yield invalid results
    case commandInProgressRegisterAddress: {
        // issue Query request
        RequestMsg rq(RequestMsgCodes::Query, 0);
        uint8_t len = protocol.EncodeRequest(rq, txbuff);
        uart.write(txbuff, len);
        RecordUARTActivity();
        // blocking wait for response
        return WaitForMMUResponse([&]() {
            out = pack_command((uint8_t)rsp.request.code, rsp.request.value); // that's the current command in progress
        });
    }

    case commandStatusRegisterAddress:
        // command status: accepted, rejected, processing, finished (raw values from the MMU protocol, correspond to ResponseMsgParamCodes
        // beware - this read register call uses a pre-cached response from read register commandInProgressRegisterAddress(253)!
        out = (uint8_t)rsp.paramCode;
        return Status::Ok;

    case commandProgressOrErrorCodeRegisterAddress:
        // progress or error code (depends on what Query returns)
        // beware - this read register call uses a pre-cached response from read register commandInProgressRegisterAddress(253)!
        out = rsp.paramValue;
        return Status::Ok;

    default: { // probably name all the remaining MMU registers here ... it's just 32 of them :)
        RequestMsg rq(RequestMsgCodes::Read, address);
        uint8_t len = protocol.EncodeRequest(rq, txbuff);
        uart.write(txbuff, len);
        RecordUARTActivity();
        // blocking wait for response
        return WaitForMMUResponse([&]() { out = rsp.paramValue; });
    }
    }
    return Status::IllegalAddress;
}

modbus::Callbacks::Status MMU::write_register(uint8_t, uint16_t address, uint16_t value) {
    uint8_t txbuff[32];
    switch (address) {
    // name writable registers explicitly and group them by 16 or 8bit writes
    // 07 | uint8  | Filament_State
    // 09 | uint8  | FSensor_State
    // 11 | uint8  | extra_load_distance
    // 12 | uint8  | FSensor_unload_check_dist.
    // 23 | uint8  | Pulley_sg_thrs__R
    // 24 | uint8  | Selector_sg_thrs_R
    // 25 | uint8  | Idler_sg_thrs_R
    // 29 | uint8  | Set/Get Selector cut iRun c
    // 35 | uint8  | Cut length
    case 7:
    case 9:
    case 11:
    case 12:
    case 23:
    case 24:
    case 25:
    case 29:
    case 35: {
        RequestMsg rq(RequestMsgCodes::Write, address, value & 0xff);
        uint8_t len = Protocol::EncodeWriteRequest(rq.value, rq.value2, txbuff);
        uart.write(txbuff, len);
        RecordUARTActivity();
        // blocking wait for response
        return WaitForMMUResponse([&]() { ; });
    }

    // 04 | uint16 | MMU_errors
    // 13 | uint16 | Pulley_unload_feedrate
    // 14 | uint16 | Pulley_acceleration
    // 15 | uint16 | Selector_acceleration
    // 16 | uint16 | Idler_acceleration
    // 17 | uint16 | Pulley_load_feedrate
    // 18 | uint16 | Selector_nominal_feedrate
    // 19 | uint16 | Idler_nominal_feedrate
    // 20 | uint16 | Pulley_slow_feedrate
    // 21 | uint16 | Selector_homing_feedrate
    // 22 | uint16 | Idler_homing_feedrate
    // 27 | uint16 | Set/Get_Selector_slot
    // 28 | uint16 | Set/Get_Idler_slot
    // 30 | uint16 | Set/Get Pulley iRun current
    // 31 | uint16 |Set/Get Selector iRun current
    // 32 | uint16 | Set/Get Idler iRun current
    // 34 | uint16 | Bowden length
    case 4:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 27:
    case 28:
    case 30:
    case 31:
    case 32:
    case 34: {
        RequestMsg rq(RequestMsgCodes::Write, address, value);
        uint8_t len = Protocol::EncodeWriteRequest(rq.value, rq.value2, txbuff);
        uart.write(txbuff, len);
        RecordUARTActivity();
        // blocking wait for response
        return WaitForMMUResponse([&]() { ; });
    }

    // virtual button register - for now write-only
    case buttonRegisterAddress: {
        RequestMsg rq(RequestMsgCodes::Button, value);
        uint8_t size = protocol.EncodeRequest(rq, txbuff);
        uart.write(txbuff, size);
        RecordUARTActivity();
        // blocking wait for response
        return WaitForMMUResponse([&]() { ; }); // response is not important, only needs to be valid
    }
    // virtual registers ... actually only 253 is RW for now
    case commandInProgressRegisterAddress: {
        // issue a command
        const auto [command, param] = unpack_command(value);
        RequestMsg rq((RequestMsgCodes)command, param);
        uint8_t len = Protocol::EncodeRequest(rq, txbuff);
        uart.write(txbuff, len);
        RecordUARTActivity();
        // blocking wait for response
        // response to commands are full of interesting data which is by-default stored in rsp
        return WaitForMMUResponse([&]() { ; });
    }

    default:
        return modbus::Callbacks::Status::IllegalAddress;
    }
}
#endif

MMU::StepStatus MMU::ExpectingMessage() {
    int bytesConsumed = 0;
    int c = -1;
    while ((c = uart.read()) >= 0) {
        ++bytesConsumed;
        switch (protocol.DecodeResponse(c)) {
        case modules::protocol::DecodeStatus::MessageCompleted:
            rsp = protocol.GetResponseMsg();
            RecordUARTActivity();
            return MessageReady;
        case modules::protocol::DecodeStatus::NeedMoreData:
            break;
        default:
            RecordUARTActivity();
            return ProtocolError;
        }
    }
    if (bytesConsumed != 0) {
        RecordUARTActivity();
        return Processing; // consumed some bytes, but message still not ready
    } else if (Elapsed(linkLayerTimeout)) {
        return CommunicationTimeout;
    }
    return Processing;
}

bool MMU::Elapsed(uint32_t timeout) const {
    return freertos::millis() >= (lastUARTActivityMs + timeout);
}

void MMU::RecordUARTActivity() {
    lastUARTActivityMs = freertos::millis();
}

int MMU2Serial::read() {
    std::byte b;
    std::span s { &b, 1 };
    auto r = hal::mmu::receive(s);
    return r.empty() ? -1 : (int)r[0];
}

void MMU2Serial::flush() {
    for (;;) {
        if (read() == -1) {
            break;
        }
    }
}

size_t MMU2Serial::write(const uint8_t *buffer, size_t size) {
    flush();
    hal::mmu::transmit(std::span<const std::byte> { (const std::byte *)buffer, size });
    return size; // a bit speculative
}
