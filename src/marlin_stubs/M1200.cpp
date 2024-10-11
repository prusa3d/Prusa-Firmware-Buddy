#include <marlin_stubs/PrusaGcodeSuite.hpp>
#include <connect/marlin_printer.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1200: Set printer ready for printing
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1200
 *
 */
void PrusaGcodeSuite::M1200() {
    if (connect_client::MarlinPrinter::is_printer_ready()) {
        SERIAL_ECHO_MSG(" M1200 - Printer is ready already");
        return;
    }

    if (!connect_client::MarlinPrinter::set_printer_ready(true)) {
        SERIAL_ERROR_MSG(" M1200 - Setting printer to ready state failed");
    }
}

/** @}*/
