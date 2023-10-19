#include "catch2/catch_test_macros.hpp"
#include "application.h"
#include <stdint.h>
#include "../modules/stubs/stub_serial.h"
#include "modules/protocol.h"

TEST_CASE("application::ReportVersion", "[application]") {
    modules::serial::ClearRX();
    modules::serial::ClearTX();

    application.ReportReadRegister(mp::RequestMsg(mp::RequestMsgCodes::Version, 0));

    REQUIRE(modules::serial::tx == "S0 A3*22\n");
}

TEST_CASE("application::WriteRegister", "[application]") {
    modules::serial::ClearRX();
    modules::serial::ClearTX();

    application.ReportWriteRegister(mp::RequestMsg(mp::RequestMsgCodes::Write, 0x1b, 2));

    REQUIRE(modules::serial::tx == "W1b A*c2\n");

    {
        modules::protocol::Protocol p;
        // verify decoding of the message - it seems we are having issues with it on the MK3 side for unknown reason
        // The MMU sends: W1c A*f0\n for some very strange reason, even though the unit tests compute a different CRC with the same piece of code!
        // Found it, see explanation in Protocol::EncodeResponseCmdAR...
        for (uint8_t i = 0; i < modules::serial::tx.size(); ++i) {
            CHECK(p.DecodeResponse(modules::serial::tx[i]) != modules::protocol::DecodeStatus::Error);
        }
    }
}
