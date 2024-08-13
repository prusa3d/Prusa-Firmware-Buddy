#include "error_code_mangle.hpp"
#include <common/printer_model.hpp>
#include <error_codes.hpp>

void update_error_code([[maybe_unused]] uint16_t &error_code) {
    error_code = error_code - (ERR_PRINTER_CODE * 1000) + (PrinterModelInfo::current().usb_pid * 1000);
}
