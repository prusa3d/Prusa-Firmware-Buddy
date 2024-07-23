#include "printer_type.hpp"
#include <config_store/store_instance.hpp>

PrinterVersion get_printer_version() {
#if PRINTER_IS_PRUSA_MK4()
    switch (config_store().extended_printer_type.get()) {

    case ExtendedPrinterType::mk3_9:
        return { 1, 3, 9 };

    case ExtendedPrinterType::mk4s:
        return { 1, 4, 1 };

    case ExtendedPrinterType::mk4:
        // Note: that would be { 1, 4, 0 } for actual mk4, which is handled
        // below.
        break;
    }
#endif

    return { printer_type, printer_version, printer_subversion };
}
