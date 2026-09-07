#pragma once

#include "screen_fsm.hpp"
#include <client_response.hpp>
#include <option/has_heaters_selftest_gcode.h>
#include <status_footer.hpp>

#if HAS_HEATERS_SELFTEST_GCODE()

/// Screen for the gcode-based heater selftest (M1987). Mirrors ScreenFanSelftest.
class ScreenHeatersSelftest final : public ScreenFSM {
public:
    ScreenHeatersSelftest();
    ~ScreenHeatersSelftest();

protected:
    void create_frame() override;
    void destroy_frame() override;
    void update_frame() override;

    PhasesHeatersSelftest get_phase() const {
        return GetEnumFromPhaseIndex<PhasesHeatersSelftest>(fsm_base_data.GetPhase());
    }

private:
    // Screen-level footer (parented to the full-screen window) showing the nozzle / bed temps.
    // Must be screen-level, not per-frame: the per-frame inner_frame excludes the footer area.
    FooterLine footer;
};

#endif // HAS_HEATERS_SELFTEST_GCODE()
