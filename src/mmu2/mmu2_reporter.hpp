/**
 * @file mmu2_reporter.hpp
 * @brief hold reporting data consistently together and handle reporting
 * do not allow direct access to member variables to keep them consistent
 * report is only set inside MMU and it is consumed elsewhere
 */

#pragma once
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_reporting.h"
#include <optional>

namespace MMU2 {

class Reporter {
public:
    using Report = std::variant<ProgressData, ErrorData>;

public:
    /// Returns the report without consuming it
    std::optional<Report> PeekReport() const {
        return report;
    }

    // These are peek convenience functions
    RawCommandInProgress GetCommandInProgress() const;
    RawProgressCode GetProgressCode() const;

    std::optional<Report> ConsumeReport() {
        std::optional<Report> r;
        r.swap(report);
        return r;
    }

    bool HasReport() const {
        return report.has_value();
    }

    void SetReport(ProgressData d);
    void SetReport(ErrorData d);

private:
    std::optional<Report> report;
};

} // namespace MMU2
