#pragma once

#include "inc/MarlinConfig.h"
#include <array>
#include <optional>

#if ENABLED(PRUSA_SPOOL_JOIN)

    #if DISABLED(PRUSA_TOOL_MAPPING)
        #error PRUSA_SPOOL_JOIN need PRUSA_TOOL_MAPPING
    #endif

/**
 * @brief Feature that allows spools to join together. When filament runs out in
          one tool, printer can continue with other tool, if spool join is configured.
 */
class SpoolJoin {
public:
    /// Spool join configuration structure
    struct __attribute__((packed)) join_config_t {
        uint8_t spool_1; // when this spool runs-out, spool2 will continue
        uint8_t spool_2;
    };

    static constexpr uint8_t reset_value { std::numeric_limits<uint8_t>::max() };

    SpoolJoin() { reset(); }

    /// Reset spool join (remove all joins)
    void reset();

    /// Add join, when spool_1 runs out, spool_2 will continue
    /// Note: spool_1/2 refers to physical tool
    bool add_join(uint8_t spool_1, uint8_t spool_2);

    // reroutes all succeeding joins to previous ones
    bool reroute_joins_containing(uint8_t spool);

    // removes whole join chain that contains given spool
    bool remove_join_chain_containing(uint8_t spool);

    // gets the first spool that will be printed with in the spool join chain, returns spool_2 if not in chain
    uint8_t get_first_spool_1_from_chain(uint8_t spool_2) const;

    // gets the spool_2 of the given spool_1
    std::optional<uint8_t> get_spool_2(uint8_t spool_1) const;

    // return number of configured joins
    inline uint8_t get_num_joins() const { return num_joins; }

    /// Get join configuration with number
    inline join_config_t get_join_nr(uint8_t join_nr) const { return joins[join_nr]; }

    /// Execute join
    bool do_join(uint8_t current_tool);
    struct __attribute__((packed)) serialized_state_t {
        join_config_t joins[EXTRUDERS];
    };
    /// Serialize data to packed serialized_state_t structure (for power panic)
    void serialize(serialized_state_t &to);

    /// deserialize data from packed serialized_state_t (recovery from power panic)
    void deserialize(serialized_state_t &from);

private:
    /// Removes join at idx from joins array and moves the last element into the idx position, so that new element can be inserted at the end again
    void remove_join_at(size_t idx);

    uint8_t num_joins; ///< Total number of joins
    std::array<join_config_t, EXTRUDERS - 1> joins; /// Configured joins, guaranteed that 0 ... num_joins - 1 are valid joins (but not any type of sort)
};

extern SpoolJoin spool_join;

#endif
