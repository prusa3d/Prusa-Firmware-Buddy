///@file
#pragma once
#include <cstdint>
#include "modbus.hpp"
#include "../../../lib/Prusa-Firmware-MMU/src/modules/protocol.h"
#include "hal.hpp"

/// A minimal serial interface for the MMU
class MMU2Serial {
public:
    MMU2Serial() = default;
    int read();
    void flush();
    size_t write(const uint8_t *buffer, size_t size);
};

class MMU final : public modbus::Callbacks {
public:
    virtual Status read_register(uint8_t, uint16_t address, uint16_t &out) override;
    virtual Status write_register(uint8_t, uint16_t address, uint16_t value) override;

private:
    modules::protocol::Protocol protocol;
    MMU2Serial uart;

    // more than 10ms doesn't make sense - we don't want to block MODBUS too much and the MMU always responds within 1ms if wires are ok
    static constexpr uint32_t linkLayerTimeout = 10;

    modules::protocol::ResponseMsg rsp = modules::protocol::ResponseMsg(modules::protocol::RequestMsg(modules::protocol::RequestMsgCodes::unknown, 0), modules::protocol::ResponseMsgParamCodes::unknown, 0);
    uint32_t lastUARTActivityMs; ///< timestamp - last ms when something occurred on the UART
    uint16_t pec;

    enum StepStatus { MessageReady,
        Processing,
        ProtocolError,
        CommunicationTimeout };
    StepStatus ExpectingMessage();

    bool Elapsed(uint32_t timeout) const;

    void RecordUARTActivity();

    template <typename F>
    Status WaitForMMUResponse(F f) {
        protocol.ResetResponseDecoder();
        for (;;) {
            switch (ExpectingMessage()) {
            case MessageReady:
                f();
                return Status::Ok;
            case Processing:
                break;
            case CommunicationTimeout:
                return Status::GatewayPathUnavailable;
            case ProtocolError:
            default:
                return Status::Ignore; // remain silent on the MODBUS, protocol_logic will retry
            }
        }
    }
};
