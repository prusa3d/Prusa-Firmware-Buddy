/**
 * @file screen_menu_diagnostics.hpp
 */

#pragma once

#include "printers.h"

// conditional parent alias include
#if PRINTER_IS_PRUSA_MINI
    #include "screen_menu_diagnostics_mini.hpp"
#endif // PRINTER_IS_PRUSA_MINI

#if PRINTER_IS_PRUSA_MK4
    #include "screen_menu_diagnostics_mk4.hpp"
#endif // PRINTER_IS_PRUSA_MK4

#if PRINTER_IS_PRUSA_MK3_5
    #include "screen_menu_diagnostics_mk3.5.hpp"
#endif // PRINTER_IS_PRUSA_MK3_5

#if PRINTER_IS_PRUSA_iX
    #include "screen_menu_diagnostics_ix.hpp"
#endif // PRINTER_IS_PRUSA_iX

#if PRINTER_IS_PRUSA_XL
    #include "screen_menu_diagnostics_xl.hpp"
#endif // PRINTER_IS_PRUSA_XL

class ScreenMenuDiagnostics : public ScreenMenuDiagnostics__ {
public:
    constexpr static const char *label = N_("DIAGNOSTICS");
    ScreenMenuDiagnostics();

private:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
