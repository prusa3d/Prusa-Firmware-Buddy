#include "connect/printer_type.hpp"

// config_store is causing build issues, let's stub this :-|

PrinterVersion get_printer_version() {
    return { PRINTER_TYPE, PRINTER_VERSION, PRINTER_SUBVERSION };
}
