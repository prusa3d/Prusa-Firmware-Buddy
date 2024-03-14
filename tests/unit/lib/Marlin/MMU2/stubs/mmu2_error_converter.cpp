#include <mmu2_error_converter.h>
#include "stub_interfaces.h"

namespace MMU2 {

uint8_t PrusaErrorCodeIndex(ErrorCode ec) {
    return 0;
}
const char *PrusaErrorTitle(uint8_t i) {
    static const char tmp[] = "err_title";
    return tmp;
}
const char *PrusaErrorDesc(uint8_t i) {
    static const char tmp[] = "err_desc";
    return tmp;
}

uint16_t PrusaErrorCode(uint8_t i) {
    return 0;
}

uint8_t PrusaErrorButtons(uint8_t i) {
    return 0;
}

const char *PrusaErrorButtonTitle(uint8_t bi) {
    static const char tmp[] = "button";
    return tmp;
}
const char *PrusaErrorButtonMore() {
    static const char tmp[] = "btnmore";
    return tmp;
}

void SetButtonResponse(ButtonOperations rsp) {
}
Buttons ButtonPressed(ErrorCode ec) {
    return Buttons::NoButton;
}
Buttons ButtonAvailable(ErrorCode ec) {
    mockLog_RecordFn();
    return Buttons::NoButton;
}

} // namespace MMU2
