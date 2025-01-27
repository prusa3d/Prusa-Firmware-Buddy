#include "PrusaGcodeSuite.hpp"

/**
 * @brief M505 is deprecated since old_eeprom -> config_store migration. The file is currently not compiled/linked and serves as a reminder that in case M505 was readded, it should be readded with the same meaning (ie setting a configuration in the printer).
 *
 */

void PrusaGcodeSuite::M505() {
    SERIAL_ERROR_MSG("M505 was deprecated");
}
