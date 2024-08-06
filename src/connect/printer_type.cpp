#include "printer_type.hpp"
#include <config_store/store_instance.hpp>

PrinterVersion get_printer_version() {
    return PrinterModelInfo::current().version;
}
