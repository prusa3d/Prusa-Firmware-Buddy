#include <marlin_stubs/PrusaGcodeSuite.hpp>
#include <feature/door_sensor_calibration/door_sensor_calibration.hpp>

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1979: Door Sensor Calibration
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1979
 *
 */
void M1980() {
    door_sensor_calibration::run();
}

/** @}*/
} // namespace PrusaGcodeSuite
