#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "PrusaGcodeSuite.hpp"
#include "../common/sys.h"
#include "../../lib/Marlin/Marlin/src/module/planner.h"
#include "../../lib/Marlin/Marlin/src/module/stepper.h"
#include "../common/variant8.h"
#include <bsod_gui.hpp>
#include <config_store/store_instance.hpp>

#ifdef Z_AXIS_CALIBRATION
void PrusaGcodeSuite::G64() {

    int additional_offset = 0;

    if (parser.seenval('D')) {
        additional_offset = parser.floatval('D');
    }

    planner.synchronize();
    int initial_steps = stepper.position_from_startup(Z_AXIS);
    if (!homeaxis(Z_AXIS)) {
        fatal_error(ErrCode::ERR_ELECTRO_HOMING_ERROR_Z);
    }
    int final_steps = stepper.position_from_startup(Z_AXIS);

    const int step_difference = std::abs(final_steps - initial_steps);

    float z_size = step_difference * planner.mm_per_step[Z_AXIS] + additional_offset;

    float old_z_size = config_store().axis_z_max_pos_mm.get();

    const float diff = std::abs(z_size - old_z_size);

    SERIAL_ECHOLNPAIR("Measured Z size ", z_size);
    SERIAL_ECHOLNPAIR("Z size in eeprom ", old_z_size);
    if (diff > (float)MIN_SAVE_DIFFERENCE) {
        config_store().axis_z_max_pos_mm.set(z_size);
    }
    SERIAL_ECHOLNPAIR("Saved Z size ", config_store().axis_z_max_pos_mm.get());
}

#endif // Z_AXIS_CALIBRATION
