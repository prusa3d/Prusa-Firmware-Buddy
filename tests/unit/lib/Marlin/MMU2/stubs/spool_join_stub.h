#pragma once
#include <stdint.h>
#include <array>
#include <optional>

class SpoolJoin {
public:
    /// Spool join configuration structure
    struct __attribute__((packed)) join_config_t {
        uint8_t spool_1; // when this spool runs-out, spool2 will continue
        uint8_t spool_2;
    };

    SpoolJoin() {}

    // gets the spool_2 of the given spool_1
    std::optional<uint8_t> get_spool_2(uint8_t spool_1) const { return std::make_optional<uint8_t>(0); }

    // return number of configured joins
    inline uint8_t get_num_joins() const { return num_joins; }

    uint8_t num_joins; ///< Total number of joins
    std::array<join_config_t, 4> joins; /// Configured joins, guaranteed that 0 ... num_joins - 1 are valid joins (but not any type of sort)
};

extern SpoolJoin spool_join;
