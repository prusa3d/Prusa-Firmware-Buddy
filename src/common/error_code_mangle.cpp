#include "error_code_mangle.hpp"

#include <cassert>
#include <utility>

#include <common/printer_model.hpp>

uint16_t map_error_code(ErrCode code) {
    const uint16_t code_num = static_cast<uint16_t>(code);
    const uint16_t printer_code = code_num / 1000;
    const uint16_t base_code = code_num % 1000;
    const uint16_t this_printer_code = PrinterModelInfo::current().error_code_prefix();

    if (code == ErrCode::ERR_UNDEF) {
        static_assert(std::to_underlying(ErrCode::ERR_UNDEF) == 0);
        // Do not map ERR_UNDEF
        return code_num;
    }

    if (printer_code == 4) {
        // Do not map MMU error codes
        return code_num;
    }

    assert(printer_code == ERR_PRINTER_CODE || printer_code == this_printer_code);
    return this_printer_code * 1000 + base_code;
}
