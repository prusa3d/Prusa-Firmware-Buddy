/// @file
#pragma once

enum class is_inverted : bool {
    no,
    yes
};

enum class has_swapped_bw : bool {
    no,
    yes
};

enum class is_shadowed : bool {
    no,
    yes
};

struct ropfn {
    is_inverted invert : 1 = is_inverted::no;
    has_swapped_bw swap_bw : 1 = has_swapped_bw::no;
    is_shadowed shadow : 1 = is_shadowed::no;
};
