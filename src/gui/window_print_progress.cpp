// window_print_progress.cpp
#include "window_print_progress.hpp"
#include "gui.hpp"
#include <algorithm>
#include "resource.h"

/*****************************************************************************/
//WindowPrintProgress
#include "marlin_client.h"
WindowPrintProgress::WindowPrintProgress(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_numberless_progress_t>(parent, rect) {}

void WindowPrintProgress::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    SuperWindowEvent(sender, event, param);
}
