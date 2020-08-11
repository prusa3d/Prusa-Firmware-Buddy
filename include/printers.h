//! @file
#pragma once

//! Printer variant
//!@{

#define PRINTER_PRUSA_MINI 2 //!< MINI printer

//!@}

#if defined(PRINTER_TYPE) && PRINTER_TYPE == PRINTER_PRUSA_MINI
    #define PRINTER_MODEL "MINI"
#else
    #error "Unknown printer type"
#endif
