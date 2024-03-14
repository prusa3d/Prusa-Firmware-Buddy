#include "connect/printer_type.hpp"

// config_store is causing build issues, let's stub this :-|

PrinterVersion get_printer_version() {
    return { printer_type, printer_version, printer_subversion };
}
