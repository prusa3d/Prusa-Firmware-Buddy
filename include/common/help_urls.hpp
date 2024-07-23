#pragma once

#include <common/extended_printer_type.hpp>

#if PRINTER_IS_PRUSA_MK4()
static constexpr EnumArray<ExtendedPrinterType, const char *, extended_printer_type_count> printer_help_url {
    { ExtendedPrinterType::mk4, "mk4" },
    { ExtendedPrinterType::mk4s, "mk4" },
    { ExtendedPrinterType::mk3_9, "mk3.9" },
};

#elif PRINTER_IS_PRUSA_MK3_5()
static constexpr const char *printer_help_url = "mk3.5";

#elif PRINTER_IS_PRUSA_XL()
static constexpr const char *printer_help_url = "xl";

#elif PRINTER_IS_PRUSA_MINI()
static constexpr const char *printer_help_url = "mini";

#elif PRINTER_IS_PRUSA_iX()
static constexpr const char *printer_help_url = "ix";

#else
    #error Please add the printer here
#endif

const char *get_printer_help_url();
