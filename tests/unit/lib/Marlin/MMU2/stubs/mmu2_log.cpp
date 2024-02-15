#define PROGMEM /**/
#include <mmu2_log.h>
#include "protocol.h"
#include "stub_interfaces.h"

MarlinLogSim marlinLogSim;

namespace MMU2 {
void LogErrorEvent_P(const char *msg_P) {
    marlinLogSim.log.push_back(msg_P);
}
void LogEchoEvent_P(const char *msg_P) {
    marlinLogSim.log.push_back(msg_P);
}

void LogRequestMsg(const char *msg) {
    REQUIRE((strlen(msg) + 1) < (uint8_t)mp::Protocol::MaxRequestSize() + 1);
    marlinLogSim.log.push_back(msg);
}
void LogResponseMsg(const char *msg) {
    REQUIRE((strlen(msg) + 1) < (uint8_t)mp::Protocol::MaxResponseSize() + 1);
    marlinLogSim.log.push_back(msg);
}

} // namespace MMU2
