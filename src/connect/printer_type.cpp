#include "printer_type.hpp"
#include <config_store/store_instance.hpp>

PrinterVersion get_printer_version() {
    PrinterVersion version;
#if PRINTER_IS_PRUSA_MK4
    // 400/200 motors are right now the way to tell if we are MK4 or MK3.9
    if (config_store().xy_motors_400_step.get()) {
        version = { printer_type, printer_version, printer_subversion };
    } else {
        version = { 1, 3, 9 };
    }
#else
    version = { printer_type, printer_version, printer_subversion };
#endif
    return version;
}
