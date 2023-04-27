/**
 * @file screen_menu_diagnostics.hpp
 */

#pragma once

#include "printers.h"

//conditional parent alias include
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "screen_menu_diagnostics_mini.hpp"
#endif //PRINTER_PRUSA_MINI

#if (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    #include "screen_menu_diagnostics_mk4.hpp"
#endif //PRINTER_PRUSA_MK4

#if (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #include "screen_menu_diagnostics_mk4.hpp" // IXL uses mk4 diagnostics, not a bug
#endif                                         //PRINTER_PRUSA_IXL

#if (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #include "screen_menu_diagnostics_xl.hpp"
#endif //PRINTER_PRUSA_XL

class ScreenMenuDiagnostics : public ScreenMenuDiagnostics__ {
public:
    constexpr static const char *label = N_("DIAGNOSTICS");
    ScreenMenuDiagnostics();

private:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
