/// @file
#pragma once

enum class has_swapped_bw : bool {
    no,
    yes
};

enum class is_shadowed : bool {
    no,
    yes
};

struct ropfn {
    has_swapped_bw swap_bw : 1 = has_swapped_bw::no;
    is_shadowed shadow : 1 = is_shadowed::no;
};
