#pragma once

#include <common/mapi/parking.hpp>
struct G27Params {

    mapi::ParkPosition where_to_park { mapi::ParkPosition::park };
    xyz_pos_t park_position { NAN, NAN, NAN };

    /// Defines how freely will Z move. For documentation see nozzle.cpp::park() or G27 marlin documentation for P parameter;
    mapi::ZAction z_action { mapi::ZAction::move_to_at_least };
};

void G27_no_parser(const G27Params &params);
