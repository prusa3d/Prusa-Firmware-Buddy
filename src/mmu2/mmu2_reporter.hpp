/**
 * @file mmu2_reporter.hpp
 * @brief hold reporting data consistently together and handle reporting
 * do not allow direct access to member variables to keep them consistent
 * report is only set inside MMU and it is consumed elsewhere
 */

#pragma once
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_reporting.h"
#include <stdint.h>
#include <optional>

namespace MMU2 {

class Reporter {
public:
    struct Error {
        ErrorCode errorCode;
        MMU2::ErrorSource errorSource;
    };
    struct Progress {
        ProgressCode progressCode; // keep it in struct to be handled the same way as error
    };
    enum class Type {
        error,
        progress
    };

    /// @brief result of reporting
    struct Report {
        CommandInProgress commandInProgress;
        Type type;
        union {
            Error error;
            Progress progress;
        };

        Report(CommandInProgress cip, Error e)
            : commandInProgress(cip)
            , type(Type::error)
            , error(e) {}
        Report(CommandInProgress cip, Progress p)
            : commandInProgress(cip)
            , type(Type::progress)
            , progress(p) {}

        bool operator==(const Report &other) const;
        bool operator!=(const Report &other) const {
            return !(*this == other);
        }
    };

private:
    Report report;
    bool reported = true; // Do we want to report initial state? If we do, change it to false;

public:
    Reporter()
        : Reporter(CommandInProgress::NoCommand, ErrorCode::OK, MMU2::ErrorSource::ErrorSourceNone) {}
    Reporter(CommandInProgress cip, ErrorCode ec, MMU2::ErrorSource es);
    Reporter(CommandInProgress cip, ProgressCode pc);

    bool operator==(const Reporter &other) const {
        return this->report == other.report;
    }
    bool operator!=(const Reporter &other) const {
        return !(*this == other);
    }

    // TODO make getter return values optional ?
    CommandInProgress GetCommand() const;
    MMU2::ErrorSource GetErrorSource() const;
    ErrorCode GetErrorCode() const;
    ProgressCode GetProgressCode() const;

    bool Change(CommandInProgress cip, ErrorCode ec, MMU2::ErrorSource es);
    bool Change(CommandInProgress cip, ProgressCode pc);

    std::optional<Report> ConsumeReport();
    bool HasReport() { return !reported; }
};

} // namespace MMU2
