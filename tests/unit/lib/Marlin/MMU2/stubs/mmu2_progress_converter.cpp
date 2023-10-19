#include <mmu2_progress_converter.h>
#include "stub_interfaces.h"

namespace MMU2 {

#define mockLog_RecordFnPc(pc) mockLog.Record(std::string { mockLog.MethodName(__PRETTY_FUNCTION__) } + "(" + std::to_string(pc) + ")")

const char *ProgressCodeToText(ProgressCode pc) {
    static const char tmp[] = "ProgressCodeToText";
    return tmp;
}

} // namespace MMU2
