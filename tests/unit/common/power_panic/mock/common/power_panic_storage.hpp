/// @file Test shadow of src/common/power_panic_storage.hpp.
///
/// The real header pulls in Marlin/HAL and cannot be compiled on host.
/// The structs here are layout stand-ins: tests assert framing-logic
/// invariants of the storage backend, never firmware sizes or offsets.
#pragma once

#include <cstddef>
#include <cstdint>

struct xy_pos_t {
    float x, y;
};

using bed_mesh_t = float[4][4];

class unified_bed_leveling {
public:
    static bed_mesh_t z_values;
};

class PrintArea {
public:
    struct rect_t {
        xy_pos_t a, b;
    };

    rect_t get_bounding_rect() const { return bounding_rect; }
    void set_bounding_rect(rect_t rect) { bounding_rect = rect; }

private:
    static inline rect_t bounding_rect {};
};

inline PrintArea print_area;

namespace power_panic {

inline constexpr size_t media_path_buffer_size = 104;

struct fixed_t {
    xy_pos_t bounding_rect_a;
    xy_pos_t bounding_rect_b;
    bed_mesh_t z_values;
    char media_SFN_path[media_path_buffer_size];

    static void load();
    static void save();
};

struct state_t {
    // odd size on purpose: catches alignment assumptions in the storage code
    uint8_t payload[219];

    static void load();
    static void save();
};

extern state_t &state_buf;

struct runtime_state_t {
    bool nested_fault;
    char media_SFN_path[media_path_buffer_size];
};

extern runtime_state_t runtime_state;

void erase();

} // namespace power_panic
