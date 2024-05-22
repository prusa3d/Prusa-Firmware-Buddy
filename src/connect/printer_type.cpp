#include "printer_type.hpp"
#include <config_store/store_instance.hpp>

PrinterVersion get_printer_version() {
#if PRINTER_IS_PRUSA_MK4
    switch (config_store().extended_printer_type.get()) {

    case ExtendedPrinterType::mk3_9:
        return { 1, 3, 9 };

    case ExtendedPrinterType::mk4:
    case ExtendedPrinterType::mk4s:
        break;
    }
#endif

    return { printer_type, printer_version, printer_subversion };
}
