/**
 * @file screen_menu_calibration.cpp
 */

#include "screen_menu_calibration.hpp"
#include "png_resources.hpp"
#include "DialogMoveZ.hpp"
#include "printers.h"

ScreenMenuCalibration::ScreenMenuCalibration()
    : ScreenMenuCalibration__(_(label)) {
#if (PRINTER_TYPE != PRINTER_PRUSA_MINI)
    header.SetIcon(&png::calibrate_white_16x16);
#endif
}

void ScreenMenuCalibration::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
