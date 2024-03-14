#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_log.h"
#include <metric.h>

namespace MMU2 {

void LogErrorEvent_P(const char *msg_P) {
    MMU2_ERROR_MSGRPGM(msg_P);
    SERIAL_ECHOLN();
}

void LogEchoEvent_P(const char *msg_P) {
    MMU2_ECHO_MSGRPGM(msg_P);
    SERIAL_ECHOLN();
}

static metric_t metric_mmu_comm = METRIC("mmu_comm", METRIC_VALUE_STRING, 0, METRIC_HANDLER_DISABLE_ALL);

void LogRequestMsg(const char *msg) {
    metric_record_string(&metric_mmu_comm, "%s", msg);
}

void LogResponseMsg(const char *msg) {
    metric_record_string(&metric_mmu_comm, "%s", msg);
}

} // namespace MMU2
