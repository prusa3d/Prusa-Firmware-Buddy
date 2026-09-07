#pragma once

#include <common/temperature.hpp>

namespace PrusaGcodeSuite {

struct M141Args {
    buddy::Temperature target_temp;
    bool wait_for_heating = false;
    bool wait_for_cooling = false;
};

/// Sets a chamber temperature to args.target_temp and optionally waits for reaching it
void M141_no_parser(const M141Args &args);

} // namespace PrusaGcodeSuite
