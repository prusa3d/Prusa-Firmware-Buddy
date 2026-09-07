/// @file
#pragma once

#include "hw_configuration_common.hpp"
#include <Pin.hpp>
#include <device/board.h>
#include <option/has_indx_head.h>

static_assert(BOARD_IS_XBUDDY());

namespace buddy::hw {

/// Abstracts runtime dynamic configuration based on incompatibilities
/// between different hardware revisions.
///
/// Used for the IXBUDDY board too, as it is very similar, but it's BoM ID is
/// different (hence the PRINTER_IS_PRUSA_iX() checks in implementation).
class Configuration : public ConfigurationCommon {
    Configuration();
    Configuration(const Configuration &) = delete;

    uint8_t loveboard_bom_id;
    bool loveboard_present;

public:
    static Configuration &Instance();

    bool has_inverted_fans() const;
    bool has_mmu_power_up_hw() const;
    bool has_trinamic_oscillators() const;
    bool check_bom_compatible() const;
    bool is_fw_compatible_with_hw() const;
    bool needs_heatbreak_thermistor_table_5() const;
    bool needs_software_mmu_powerup() const;
    float curr_measurement_voltage_to_current(float voltage) const;

    /// Configures the ext_reset pin correctly based on the revision
    void setup_ext_reset() const;

    /// Activates the ext_reset pin correctly based on the revision
    void activate_ext_reset() const;

    /// Deactivates ext_reset pin correctly based on the revision
    void deactivate_ext_reset() const;

#if HAS_INDX_HEAD()
    /// Writes the indx_head_reset pin (State::high = reset asserted), compensating for revision-dependent polarity
    void write_indx_head_reset(Pin::State state) const;
#endif

private:
    bool has_inverted_mmu_reset() const;
    bool needs_push_pull_mmu_reset_pin() const;
};

} // namespace buddy::hw
