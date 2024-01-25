#include "mmu2_reporter.hpp"
#include "log.h"

LOG_COMPONENT_REF(MMU2);

namespace MMU2 {

RawCommandInProgress Reporter::GetCommandInProgress() const {
    const auto r = PeekReport();
    if (!r) {
        return ftrstd::to_underlying(CommandInProgress::NoCommand);
    }

    return std::visit([](auto &&r) -> RawCommandInProgress {
        return r.rawCommandInProgress;
    },
        *r);
}
RawProgressCode Reporter::GetProgressCode() const {
    const auto r = PeekReport();
    if (!r || !std::holds_alternative<ProgressData>(*r)) {
        return ftrstd::to_underlying(ProgressCode::OK);
    }

    return std::get<ProgressData>(*r).rawProgressCode;
}

void Reporter::SetReport(ProgressData d) {
    if (report == Report(d)) {
        return;
    }

    report = d;
    log_debug(MMU2, "MMU progress set: cip: %d pc: %u", d.rawCommandInProgress, d.rawProgressCode);
}
void Reporter::SetReport(ErrorData d) {
    if (report == Report(d)) {
        return;
    }

    report = d;
    log_debug(MMU2, "MMU error set: cip: %d ec: %u es: %d", d.rawCommandInProgress, static_cast<unsigned>(d.errorCode), static_cast<unsigned>(d.errorSource));
}

} // namespace MMU2
