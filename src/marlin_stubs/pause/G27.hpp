#pragma once
struct G27Params {
    bool do_x { false };
    bool do_y { false };
    bool do_z { false };
    xyz_pos_t park_position {};
    /// Defines how freely will Z move. For documentation see nozzle.cpp::park() or G27 marlin documentation for P parameter;
    uint16_t z_action { 0 };
};

void G27_no_parser(const G27Params &params);
