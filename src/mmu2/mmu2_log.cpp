#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_log.h"

namespace MMU2 {

void LogErrorEvent_P(const char *msg_P) {
    MMU2_ERROR_MSGRPGM(msg_P);
    SERIAL_ECHOLN();
}

void LogEchoEvent_P(const char *msg_P) {
    MMU2_ECHO_MSGRPGM(msg_P);
    SERIAL_ECHOLN();
}

} // namespace MMU2
