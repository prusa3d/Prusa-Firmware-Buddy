#include <logging/log.h>

LOG_COMPONENT_REF(PRUSA_GCODE);

namespace PrusaGcodeSuite {

void M704() { log_error(PRUSA_GCODE, "M704 unsupported"); }
void M705() { log_error(PRUSA_GCODE, "M705 unsupported"); }
void M706() { log_error(PRUSA_GCODE, "M706 unsupported"); }
void M707() { log_error(PRUSA_GCODE, "M707 unsupported"); }
void M708() { log_error(PRUSA_GCODE, "M708 unsupported"); }
void M709() { log_error(PRUSA_GCODE, "M709 unsupported"); }
void M1704() { log_error(PRUSA_GCODE, "M1704 unsupported"); }

} // namespace PrusaGcodeSuite
