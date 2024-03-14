#include "mmu2_reporter.hpp"
#include "log.h"

LOG_COMPONENT_REF(MMU2);

namespace MMU2 {

Reporter::Reporter(CommandInProgress cip, ErrorCode ec, MMU2::ErrorSource es)
    : report(cip, { ec, es }) {}

Reporter::Reporter(CommandInProgress cip, ProgressCode pc)
    : report(cip, { pc }) {}

bool Reporter::Report::operator==(const Report &other) const {
    if (commandInProgress != other.commandInProgress) {
        return false;
    }

    // no command is being processed, other variables are irrelevant
    // but error from MMU can have CommandInProgress::NoCommand and still be valid !!!
    if (commandInProgress == CommandInProgress::NoCommand && error.errorSource == other.error.errorSource) {
        return true;
    }

    if (type != other.type) {
        return false;
    }

    // handle progress
    if (type == Type::progress) {
        return progress.progressCode == other.progress.progressCode;
    }

    // now we know type is error
    if (error.errorCode != other.error.errorCode) {
        return false;
    }

    // no active error, other variables are irrelevant
    if (error.errorCode == ErrorCode::OK || error.errorCode == ErrorCode::RUNNING) {
        return true;
    }

    // we have an error and error codes match, sources must match too
    return error.errorSource == other.error.errorSource;
}

CommandInProgress Reporter::GetCommand() const {
    return report.commandInProgress;
}

MMU2::ErrorSource Reporter::GetErrorSource() const {
    return report.type == Type::error ? report.error.errorSource : MMU2::ErrorSource::ErrorSourceNone;
}

ErrorCode Reporter::GetErrorCode() const {
    return report.type == Type::error ? report.error.errorCode : ErrorCode::RUNNING;
}

ProgressCode Reporter::GetProgressCode() const {
    return report.type == Type::progress ? report.progress.progressCode : ProgressCode::Empty;
}

/**
 * @brief change state
 *
 * @param cip
 * @param ec
 * @return true  changed
 * @return false not changed
 */
bool Reporter::Change(CommandInProgress cip, ErrorCode ec, MMU2::ErrorSource es) {
    const Reporter cs(cip, ec, es);
    if (*this == cs) {
        return false;
    }
    *this = cs;
    reported = false;
    log_error(MMU2, "MMU error set: cip: %d ec: %d es: %d", cip, ec, es);
    return true;
}

/**
 * @brief change state
 *
 * @param cip
 * @param pc
 * @return true  changed
 * @return false not changed
 */
bool Reporter::Change(CommandInProgress cip, ProgressCode pc) {
    const Reporter cs(cip, pc);
    if (*this == cs) {
        return false;
    }
    *this = cs;
    reported = false;
    log_info(MMU2, "MMU progress set: cip: %d pc: %d", cip, pc);
    return true;
}

std::optional<Reporter::Report> Reporter::ConsumeReport() {
    if (reported) {
        return std::nullopt;
    }

    reported = true;
    return report;
}

} // namespace MMU2
