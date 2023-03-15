#pragma once

#include "puppies/fifo_coder.hpp"

using namespace common::puppies::fifo;

struct LoadcellRecord {
    TimeStamp_us_t timestamp;
    LoadCellData_t loadcell_raw_value;
};
