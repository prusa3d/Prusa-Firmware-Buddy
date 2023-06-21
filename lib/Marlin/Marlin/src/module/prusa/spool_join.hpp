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
    struct join_config_t {
        uint8_t spool_1; // when this spool runs-out, spool2 will continue
        uint8_t spool_2;
    };

    SpoolJoin() { reset(); }

    /// Reset spool join (remove all joins)
    void reset();

    /// Add join, when spool_1 runs out, spool_2 will continue
    /// Note: spool_1/2 refers to physical tool
    bool add_join(uint8_t spool_1, uint8_t spool_2);

    // return number of configured joins
    inline uint8_t get_num_joins() { return num_joins; }

    /// Get join configuration with number
    inline join_config_t get_join_nr(uint8_t join_nr) { return joins[join_nr]; }

    /// Execute join
    bool do_join(uint8_t current_tool);

private:
    uint8_t num_joins;                              ///< Total number of joins
    std::array<join_config_t, EXTRUDERS - 1> joins; /// Configured joins

    std::optional<uint8_t> get_join_for_tool(uint8_t tool);
};

extern SpoolJoin spool_join;

#endif
