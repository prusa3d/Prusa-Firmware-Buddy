/**
 * @file
 * @date Aug 23, 2021
 * @author Marek Bel
 */

#include "../gcode.h"
#include "../../module/stepper.h"

/**
 * @brief Enable independent XY stepping
 */
void GcodeSuite::M170() {
  Stepper::independent_XY_stepping_enabled = true;
}


/**
 * @brief Disable independent XY stepping
 */
void GcodeSuite::M171() {
  Stepper::independent_XY_stepping_enabled = false;
}
