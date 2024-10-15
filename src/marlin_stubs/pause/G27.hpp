#pragma once
struct G27Params {

    enum class ParkPosition : uint8_t {
        park,
        purge,
        load,
        _cnt,
    };

    ParkPosition where_to_park { ParkPosition::park };
    xyz_pos_t park_position { NAN, NAN, NAN };

    /// Defines how freely will Z move. For documentation see nozzle.cpp::park() or G27 marlin documentation for P parameter;
    uint16_t z_action { 0 };
};

void G27_no_parser(const G27Params &params);
