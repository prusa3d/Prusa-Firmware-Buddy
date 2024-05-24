#pragma once

#include <common/selftest/i_selftest_part.hpp>

namespace selftest {

enum class RevisePrinterSetupResult {
    running, ///< Selftest part still running
    do_not_retry, ///< Do not retry the failed selftest
    retry, ///< Retry the failed selftest
};

RevisePrinterSetupResult phase_revise_printer_setup();

} // namespace selftest
